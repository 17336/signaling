#include "connectionPool.h"

#include <stdio.h>

#include <exception>
#include <mutex>
#include <stdexcept>

log4cxx::LoggerPtr ConnPool::logger_ = log4cxx::Logger::getLogger("server");

// 获取连接池对象，单例模式
ConnPool* ConnPool::getInstance() {
    static ConnPool connPool("tcp://127.0.0.1:3306", "sig", "12345678", 20);
    return &connPool;
}

// 数据库连接池的构造函数
ConnPool::ConnPool(std::string url, std::string username, std::string password,
                   int max_size) {
    max_size_ = max_size;
    cur_size_ = 0;
    username_ = username;
    password_ = password;
    url_ = url;

    try {
        driver_ = sql::mysql::get_driver_instance();
    } catch (sql::SQLException& e) {
        LOG4CXX_ERROR(logger_, "get driver_ error.");
    } catch (std::runtime_error& e) {
        LOG4CXX_ERROR(logger_, "[ConnPool] run time error.");
    }

    // 在初始化连接池时，建立一定数量的数据库连接
    initConnection(max_size_ / 2);
}

// 初始化数据库连接池，创建最大连接数一半的连接数量
void ConnPool::initConnection(int iInitialSize) {
    sql::Connection* conn;
    std::lock_guard<std::mutex> lock(m_);
    for (int i = 0; i < iInitialSize; i++) {
        conn = createConnection();
        if (conn && conn->isValid()) {
            auto stmt = conn->createStatement();
            stmt->execute("USE db_signaling");
            delete stmt;
            connlist_.push_back(conn);
            ++(cur_size_);
        } else {
            LOG4CXX_ERROR(logger_, "Init connection error.");
        }
    }
}

// 创建并返回一个连接
sql::Connection* ConnPool::createConnection() {
    sql::Connection* conn;
    try {
        // 建立连接
        conn = driver_->connect(url_, username_, password_);
        return conn;
    } catch (sql::SQLException& e) {
        LOG4CXX_ERROR(logger_, "create connection error.");
        return nullptr;
    } catch (std::runtime_error& e) {
        LOG4CXX_ERROR(logger_, "[createConnection] run time error.");
        return nullptr;
    }
}

// 从连接池中获得一个连接
sql::Connection* ConnPool::getConnection() {
    sql::Connection* con;
    std::lock_guard<std::mutex> lock(m_);

    // 连接池容器中还有连接
    if (connlist_.size() > 0) {
        // 获取第一个连接
        con = connlist_.front();
        // 移除第一个连接
        connlist_.pop_front();
        // 判断获取到的连接的可用性
        // 如果连接已经被关闭，删除后重新建立一个
        if (con->isClosed()) {
            delete con;
            con = createConnection();
            // 如果连接为空，说明创建连接出错
            if (con == nullptr) {
                // 从容器中去掉这个空连接
                --cur_size_;
            }
        }
        return con;
    }
    // 连接池容器中没有连接
    else {
        // 当前已创建的连接数小于最大连接数，则创建新的连接
        if (cur_size_ < max_size_) {
            con = createConnection();
            if (con) {
                ++cur_size_;
                return con;
            } else {
                return nullptr;
            }
        }
        // 当前建立的连接数已经达到最大连接数
        else {
            LOG4CXX_ERROR(logger_,
                          "[getConnection] connections reach the max number.");
            return nullptr;
        }
    }
}

// 释放数据库连接，将该连接放回到连接池中
void ConnPool::releaseConnection(sql::Connection* conn) {
    std::lock_guard<std::mutex> lock(m_);
    if (conn && !conn->isClosed()) {
        connlist_.push_back(conn);
    } else {
        --cur_size_;
    }
}

// 数据库连接池的析构函数
ConnPool::~ConnPool() { destoryConnPool(); }

// 销毁连接池，需要先销毁连接池的中连接
void ConnPool::destoryConnPool() {
    std::list<sql::Connection*>::iterator itCon;
    std::lock_guard<std::mutex> lock(m_);

    for (itCon = connlist_.begin(); itCon != connlist_.end(); ++itCon) {
        // 销毁连接池中的连接
        destoryConnection(*itCon);
    }
    cur_size_ = 0;
    // 清空连接池中的连接
    connlist_.clear();
}

// 销毁数据库连接
void ConnPool::destoryConnection(sql::Connection* conn) {
    if (conn) {
        try {
            // 关闭连接
            conn->close();
        } catch (sql::SQLException& e) {
            LOG4CXX_ERROR(logger_, e.what());
        } catch (std::exception& e) {
            LOG4CXX_ERROR(logger_, e.what());
        }
        // 删除连接
        delete conn;
    }
}