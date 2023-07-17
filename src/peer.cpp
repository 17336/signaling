#include "peer.h"

peer::peer() {
    logger=log4cxx::Logger::getRootLogger();
}

peer::peer(server *s, const int64_t &id,
           const connection_hdl &con, const std::string &name) :s_(s), id_(id), con_(con), name_(name) {
    logger=log4cxx::Logger::getRootLogger();
}

peer::~peer() {
}

peer::connection_hdl& peer::getCon() {
    return con_;
}

bool peer::sendMsg(message_ptr msg) {
    error_code res_code;
    s_->send(con_, msg, res_code);
    if (res_code) {
        LOG4CXX_WARN(logger, "failed to send msg to "<<id_<<", code: "<< res_code.value());
        return false;
    }
    return true;
}

void peer::sendMsg(const std::string& msg){
    s_->send(con_, msg, opcode::TEXT);
}