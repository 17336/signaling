#ifndef _ROOM_H_
#define _ROOM_H_

#include <log4cxx/basicconfigurator.h>

#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

#include "peer.h"
#include "rapidjson/document.h"
#include "rapidjson/rapidjson.h"
#include "session.h"
#include "type.h"

class RoomManager;

class Room {
    friend class RoomManager;

public:
    Room(int64_t id);
    Room(const Room& other);
    Room(Room&& other);
    Room& operator=(const Room&);
    ~Room();

    bool addPeer(int64_t pid, std::shared_ptr<Peer>);
    bool removePeer(int64_t from_pid);
    bool sendToRoom(int64_t from_pid, const std::string& msg);
    bool isInroom(int64_t from_pid);
    bool empty();

    void getPeers(rapidjson::Document& d,
                  rapidjson::Document::AllocatorType& allocator);
    int64_t getID() const { return id_; };

private:
    std::unordered_map<int64_t, std::shared_ptr<Peer>> peers_;
    std::mutex mu_;
    int64_t id_;
    Session session_;
    static log4cxx::LoggerPtr logger_;
};

#endif  // _ROOM_H_