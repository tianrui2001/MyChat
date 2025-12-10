#include "connectionpool.hpp"
#include <iostream>
#include <mysql/mysql.h>

ConnectionPool* ConnectionPool::getInstance() {
    static ConnectionPool pool;
    return &pool;
}

// 构造函数：初始化配置并创建初始连接
ConnectionPool::ConnectionPool() {
    // 1. 这里应该读取配置文件，为了简单我先硬编码，你自己改成读配置
    _ip = server;
    _port = 3306;
    _user = user;       // 你的数据库账号
    _password = password;    // 你的数据库密码
    _dbname = dbname;     // 你的数据库名
    _initSize = initSize;        // 初始连接数
    _maxSize = maxSize;        // 最大连接数
    _connectionCnt = 0;   // 当前总连接数

    // 2. 预先创建连接
    for (int i = 0; i < _initSize; ++i) {
        MYSQL* conn = createConnection();
        if (conn) {
            _connQueue.push(conn);
            _connectionCnt++;
        }
    }
}

MYSQL* ConnectionPool::createConnection() {
    MYSQL* conn = mysql_init(nullptr);
    // 自动重连设置
    char reconnect = 1;
    mysql_options(conn, MYSQL_OPT_RECONNECT, &reconnect);   // 触发自动重连，防止崩溃
    
    conn = mysql_real_connect(conn, _ip.c_str(), _user.c_str(), 
                              _password.c_str(), _dbname.c_str(), 
                              _port, nullptr, 0);
    if(conn != nullptr) {
        // 设置编码，防止中文乱码
        mysql_query(conn, "set names gbk"); 
    }
    return conn;
}

// // 获取连接（消费者）
MYSQL* ConnectionPool::getConnection() {
    unique_lock<mutex> lock(_queueMutex);

    // 情况 1：队列里没连接了
    if (_connQueue.empty()) {
        
        // 如果当前连接数还没达到最大值，说明还可以新建！
        if (_connectionCnt < _maxSize) { 
            // 应该解锁创建连接，避免阻塞其他线程获取连接
            MYSQL* conn = createConnection();
            if (conn != nullptr) {
                _connectionCnt++; 
                return conn;      // 直接把新连接给用户，不用入队
            }
        }

        // 如果连接数已经达到 _maxSize 了，或者创建失败, 只能死等别人释放连接
        while (_connQueue.empty()) {
            if (_cv.wait_for(lock, chrono::milliseconds(100)) == cv_status::timeout) {
                 if (_connQueue.empty()) {
                     return nullptr;
                 }
            }
        }
    }

    // 情况 2：队列里有空闲连接，直接取
    MYSQL* conn = _connQueue.front();
    _connQueue.pop();
    return conn;
}

// 归还连接（生产者）
void ConnectionPool::releaseConnection(MYSQL* conn) {
    if (conn == nullptr) return;

    unique_lock<mutex> lock(_queueMutex);
    _connQueue.push(conn);
    _cv.notify_all(); // 通知等待的线程
}

ConnectionPool::~ConnectionPool() {
    // 释放所有连接资源
    unique_lock<mutex> lock(_queueMutex);
    while (!_connQueue.empty()) {
        MYSQL* conn = _connQueue.front();
        _connQueue.pop();
        mysql_close(conn);
    }
}