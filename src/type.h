#ifndef _TYPE_H_
#define _TYPE_H_

#include <websocketpp/connection.hpp>         // connection_hdl,message_ptr
#include <websocketpp/server.hpp>             // server
#include <websocketpp/config/asio_no_tls.hpp> // websocketpp::config::asio

class Type{
public:
    typedef websocketpp::server<websocketpp::config::asio> server;
    typedef websocketpp::connection_hdl connection_hdl;
    typedef server::connection_ptr connection_ptr;
    typedef server::message_ptr message_ptr;
    typedef websocketpp::frame::opcode::value opcode;
    typedef websocketpp::lib::error_code error_code;    
};


#endif // _TYPE_H_