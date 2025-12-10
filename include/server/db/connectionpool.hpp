#pragma once

#include <string>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <mysql/mysql.h> // 原生 MySQL 头文件
#include <atomic>

using namespace std;

static string server = "127.0.0.1";
static string user = "root";
static string password = "123456";
static string dbname = "chat";
static int initSize = 10;
static int maxSize = 15;

// 数据库连接池（单例模式）
class ConnectionPool {
public:
    // 获取单例实例
    static ConnectionPool* getInstance();

    // 从连接池获取一个可用连接
    MYSQL* getConnection();

    // 归还连接到连接池
    void releaseConnection(MYSQL* conn);

private:
    ConnectionPool(); // 构造函数私有化
    ~ConnectionPool();

    // 真正创建连接的辅助函数
    MYSQL* createConnection();

    // 配置参数
    string _ip;
    unsigned short _port;
    string _user;
    string _password;
    string _dbname;

    int _initSize;  // 初始连接数
    int _maxSize;   // 最大连接数
    atomic_int _connectionCnt; // 记录当前总连接数

    queue<MYSQL*> _connQueue; // 连接队列
    mutex _queueMutex;        // 队列锁
    condition_variable _cv;   // 条件变量，用于等待连接
};
