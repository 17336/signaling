#ifndef _PEER_H_
#define _PEER_H_

#include <string>
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/connection.hpp>
#include <websocketpp/endpoint.hpp>
#include <websocketpp/server.hpp>

#include "log4cxx/log4cxx.h"
#include "log4cxx/logger.h"
#include "type.h"
#include "peerStatus.h"

class PeerStatus;

class Peer {
public:
    Peer(const int64_t& id, const Type::connection_ptr& con,
         const std::string& name);
    Peer(const Peer& other);
    Peer& operator=(const Peer& other);
    Peer(Peer&& other);

    Type::connection_ptr getCon();
    bool sendMsg(Type::message_ptr msg);
    bool sendMsg(const std::string& msg);
    ~Peer();
    std::string name() const { return name_; }
    std::string ip() const { return ip_; }
    int64_t id() const { return id_; }

    PeerStatus peer_status_;

private:
    int64_t id_;
    std::string name_;
    std::string ip_;
    Type::connection_ptr con_;
    static log4cxx::LoggerPtr logger_;
};

#endif  // _PEER_H_
