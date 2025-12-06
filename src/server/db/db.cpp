#include "db.h"
#include <muduo/base/Logging.h>
#include <string>

using namespace std;

static string server = "127.0.0.1";
static string user = "root";
static string password = "123456";
static string dbname = "chat";

// 初始化连接
MySQL::MySQL()
{
    _conn = mysql_init(nullptr);    // 分配一些空间
}

// 释放连接
MySQL::~MySQL()
{
    if(_conn != nullptr)
    {
        mysql_close(_conn);
    }
}

// 连接数据库
bool MySQL::connect()
{
    MYSQL *p = mysql_real_connect(_conn, server.c_str(),
                                   user.c_str(), password.c_str(),
                                   dbname.c_str(), 3306, nullptr, 0);
    if(p != nullptr){
         mysql_query(_conn, "set names gbk"); // 设置字符集， C/C++ 默认是 ASCII,不设置中文会显示？
         LOG_INFO << "MySQL Connect Success!";
    }else{
        LOG_INFO << "MySQL Connect Fail!";
    }
    return p != nullptr;
}

// 更新操作
bool MySQL::update(string sql)
{
    if(mysql_query(_conn, sql.c_str())){
        LOG_INFO << __FILE__ << " : " << __LINE__ << " : " << sql << "更新失败！";
        return false;
    }

    return true;
}

// 查询操作
MYSQL_RES* MySQL::query(string sql)
{
    if(mysql_query(_conn, sql.c_str())){
        LOG_INFO << __FILE__ << " : " << __LINE__ << " : " << sql << "查询失败！";
        return nullptr;
    }
    return mysql_use_result(_conn);
}

MYSQL* MySQL::getConnection()
{
    return _conn;
}