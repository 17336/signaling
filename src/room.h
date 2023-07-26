#ifndef _ROOM_H_
#define _ROOM_H_

#include <unordered_map>
#include <unordered_set>
#include <string>
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include <mutex>
#include <log4cxx/basicconfigurator.h>  
#include "peer.h"


class room
{
    friend class sigServer;
public:
    typedef websocketpp::connection_hdl connection_hdl;
    typedef websocketpp::lib::mutex mutex;
    void addPeer(int64_t pid);
    void removePeer(int64_t pid);
    bool empty();
    room(int64_t id);
    room(const room & other);
    room& operator=(const room &);
    room(){};
    ~room();

private:
    std::unordered_set<int64_t> pids_;
    mutex pids_mutex_;
    int64_t id_;
    log4cxx::LoggerPtr logger;
};

#endif // _ROOM_H_