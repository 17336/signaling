#include "room.h"


using websocketpp::lib::bind;
using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;

using websocketpp::lib::condition_variable;
using websocketpp::lib::lock_guard;
using websocketpp::lib::thread;
using websocketpp::lib::unique_lock;

room::room(int64_t id) : id_(id) {
    logger=log4cxx::Logger::getRootLogger();
}

room::room(const room & other) {
    logger=log4cxx::Logger::getRootLogger();
    this->id_ = other.id_;
    this->pids_ = other.pids_;
}

room& room::operator=(const room &other) {
    this->id_ = other.id_;
    this->pids_ = other.pids_;
    return *this;
}

room::~room() {}

void room::addPeer(int64_t pid) {
    lock_guard<mutex> lock(pids_mutex_);
    if (pids_.find(pid) != pids_.end()) {
        LOG4CXX_WARN(logger, pid<<" already in room");
        return;
    }
    pids_.insert(pid);
}

void room::removePeer(int64_t pid)
{
    lock_guard<mutex> lock(pids_mutex_);
    if (pids_.find(pid) == pids_.end()) {
        LOG4CXX_WARN(logger, pid<<" not in room");
        return;
    }
    pids_.erase(pid);
}