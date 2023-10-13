#ifndef _CONNECTIONPOOL_H_
#define _CONNECTIONPOOL_H_

#include <cppconn/connection.h>
#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include <mysql_connection.h>
#include <mysql_driver.h>
#include "log4cxx/logger.h"

#include <list>
#include <mutex>
#include <string>

class ConnPool {
public:
    ~ConnPool();
    // 获取数据库连接
    sql::Connection* getConnection();
    // 将数据库连接放回到连接池的容器中
    void releaseConnection(sql::Connection* conn);
    // 获取数据库连接池对象
    static ConnPool* getInstance();

private:
    // 当前已建立的数据库连接数量
    int cur_size_;
    // 连接池定义的最大数据库连接数
    int max_size_;
    std::string username_;
    std::string password_;
    std::string url_;
    // 连接池容器
    std::list<sql::Connection*> connlist_;
    // 线程锁
    std::mutex m_;
    sql::Driver* driver_;
    static log4cxx::LoggerPtr logger_;

    // 创建一个连接
    sql::Connection* createConnection();
    // 初始化数据库连接池
    void initConnection(int iInitialSize);
    // 销毁数据库连接对象
    void destoryConnection(sql::Connection* conn);
    // 销毁数据库连接池
    void destoryConnPool();
    // 构造方法
    ConnPool(std::string url, std::string username, std::string password, int max_size);
};

#endif  // _CONNECTIONPOOL_H_