#ifndef _SIGSERVER_H_
#define _SIGSERVER_H_

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include <iostream>
#include <unordered_map>
#include <vector>
#include "peer.h"
#include "room.h"
#include "log4cxx/logger.h"
#include <queue>

class sigServer
{
public:
    typedef websocketpp::server<websocketpp::config::asio> server;
    typedef websocketpp::connection_hdl connection_hdl;
    // pull out the type of messages sent by our config
    typedef server::message_ptr message_ptr;
    typedef websocketpp::lib::error_code error_code;
    typedef websocketpp::lib::mutex mutex;
    typedef websocketpp::lib::condition_variable condition_variable;
    typedef websocketpp::frame::opcode::value opcode;

    struct task
    {
        task(connection_hdl h) : hdl(h) {}
        task(connection_hdl h, server::message_ptr m)
            : hdl(h), msg(m) {}

        websocketpp::connection_hdl hdl;
        server::message_ptr msg;
    };

    enum operate
    {
        LOG_IN = 0,
        LOG_OUT = 1,
        CREATE_ROOM = 2,
        JOIN_ROOM = 3,
        LEFT_ROOM = 4,
        SEND_TO = 5,
        SEND_TO_ROOM = 6,
        GET_PEERS_IN_ROOM=7,
    };

    sigServer(/* args */);
    bool checkLog(connection_hdl hdl, int64_t pid);
    bool checkCreate(connection_hdl hdl, int64_t rid);
    // 用户注册id
    void logIn(connection_hdl hdl, int64_t pid, const std::string& name);
    void logOut(connection_hdl hdl, int64_t pid);

    void createRoom(connection_hdl hdl, int64_t pid);
    void joinRoom(connection_hdl hdl, int64_t pid, int64_t rid);
    void leftRoom(connection_hdl hdl, int64_t pid, int64_t rid);
    // 给pid发消息
    void sendTo(connection_hdl hdl, int64_t pid, std::string msg);
    // 给room发消息
    void sendToRoom(connection_hdl hdl, int64_t rid, std::string msg);
    // 给room发消息除了from_pid
    void sendToRoom(connection_hdl hdl, int64_t from_pid, int64_t rid, std::string msg);
    void getPeersInRoom(connection_hdl hdl, int64_t rid);
    void processTasks();

    void on_open(connection_hdl hdl);
    void on_close(connection_hdl hdl);
    void on_message(connection_hdl hdl, server::message_ptr msg);

    void run(uint16_t port);

private:
    server m_server_;

    std::unordered_map<int64_t, peer> peers_;
    mutex peer_lock_;
    std::unordered_map<int64_t, room> rooms_;
    mutex room_lock_;
    int64_t next_peer_id_;
    int64_t next_room_id_;

    std::queue<task> tasks_;
    mutex tasks_lock_;
    condition_variable tasks_cond_;
    log4cxx::LoggerPtr logger;
};

#endif // _SIGSERVER_H_