#include "peer.h"

log4cxx::LoggerPtr Peer::logger_ = log4cxx::Logger::getLogger("processor");

Peer::Peer(const int64_t &id, const Type::connection_ptr &con,
           const std::string &name)
    : id_(id), con_(con), name_(name) {
    // maybe throw exception, should catch
    ip_ = con_->get_remote_endpoint();
}

Peer::Peer(const Peer &other) {
    id_ = other.id_;
    con_ = other.con_;
    ip_ = other.ip_;
    name_ = other.name_;
}

Peer &Peer::operator=(const Peer &other) {
    id_ = other.id_;
    ip_ = other.ip_;
    con_ = other.con_;
    name_ = other.name_;
    return *this;
}

// 夺取右值，避免拷贝
Peer::Peer(Peer &&other) : con_(other.con_), id_(other.id_) {
    ip_.swap(other.ip_);
    name_.swap(other.name_);
    other.con_.reset();
}

Peer::~Peer() {}

Type::connection_ptr Peer::getCon() { return con_; }

bool Peer::sendMsg(Type::message_ptr msg) {
    Type::error_code res_code = con_->send(msg);
    if (res_code) {
        LOG4CXX_WARN(logger_, "failed to send msg to "
                                  << id_ << ", code: " << res_code.value());
        return false;
    }
    return true;
}

bool Peer::sendMsg(const std::string &msg) {
    Type::error_code res_code = con_->send(msg, Type::opcode::TEXT);
    if (res_code) {
        LOG4CXX_WARN(logger_, "failed to send msg to "
                                  << id_ << ", code: " << res_code.value());
        return false;
    }
    return true;
}