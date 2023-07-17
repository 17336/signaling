#ifndef _PEER_H_
#define _PEER_H_

#include <string>
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include "log4cxx/log4cxx.h"
#include "log4cxx/logger.h"

class peer
{
public:
    typedef websocketpp::server<websocketpp::config::asio> server;
    typedef websocketpp::connection_hdl connection_hdl;
    // pull out the type of messages sent by our config
    typedef server::message_ptr message_ptr;
    typedef websocketpp::frame::opcode::value opcode;
    typedef websocketpp::lib::error_code error_code;
    peer(server *s, const int64_t &id, const connection_hdl & con, const std::string &name);
    peer();
    connection_hdl& getCon();
    bool sendMsg(message_ptr msg);
    void sendMsg(const std::string& msg);
    ~peer();
private:
    int64_t id_;
    std::string name_;
    connection_hdl con_;
    server *s_;
    log4cxx::LoggerPtr logger;
};

#endif // _PEER_H_
