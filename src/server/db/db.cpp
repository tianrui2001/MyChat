#include <string>
#include <muduo/base/Logging.h>

#include "db.hpp"
#include "connectionpool.hpp" // 引入连接池


using namespace std;

// 构造函数：向连接池申请连接
MySQL::MySQL()
{
    // 核心：直接从单例连接池里拿
    _conn = ConnectionPool::getInstance()->getConnection();
}

// 析构函数：将连接归还给连接池
MySQL::~MySQL()
{
    if (_conn != nullptr)
    {
        // 核心：用完不销毁，而是还回去
        ConnectionPool::getInstance()->releaseConnection(_conn);
    }
}

// 现在 connect 不需要建立 TCP 了，只需要判断是否拿到连接即可
bool MySQL::connect()
{
    if (_conn == nullptr)
    {
        LOG_INFO << "ConnectionPool empty, get connection failed!";
        return false;
    }
    return true;
}

// 更新操作
bool MySQL::update(string sql)
{
    if (mysql_query(_conn, sql.c_str()))
    {
        LOG_INFO << __FILE__ << ":" << __LINE__ << ":" << sql << " 更新失败! " << mysql_error(_conn);
        return false;
    }
    return true;
}

// 查询操作
MYSQL_RES* MySQL::query(string sql)
{
    if (mysql_query(_conn, sql.c_str()))
    {
        LOG_INFO << __FILE__ << ":" << __LINE__ << ":" << sql << " 查询失败! " << mysql_error(_conn);
        return nullptr;
    }
    return mysql_use_result(_conn);
}

MYSQL* MySQL::getConnection()
{
    return _conn;
}