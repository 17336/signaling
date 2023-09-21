#include "roomManager.h"

#include <memory>
#include <string>

#include "peer.h"
#include "peerManager.h"
#include "util.h"

RoomManager* RoomManager::getInstance() {
    static RoomManager room_manager;
    return &room_manager;
}

RoomManager::RoomManager() : next_id_(0) {}

log4cxx::LoggerPtr RoomManager::logger_ = log4cxx::Logger::getRootLogger();

void RoomManager::searchRoom(Type::connection_ptr con, int64_t rid,
                             int64_t from_pid) {
    LOG4CXX_INFO(logger_,
                 "from_pid: " << from_pid << " want to search room: " << rid);
    std::shared_ptr<Room> room = getRoom(rid);
    if (!room) {
        response(con, "success",
                 {"type", "searchRoom", "rid", std::to_string(rid), "exist",
                  "false"});
    } else {
        response(con, "success",
                 {"type", "searchRoom", "rid", std::to_string(rid), "exist",
                  "true"});
    }
}

void RoomManager::createRoom(Type::connection_ptr con, int64_t from_pid) {
    LOG4CXX_INFO(logger_, "from_pid: " << from_pid << " want to create room");
    std::shared_ptr<Peer> peer = PeerManager::getInstance()->getPeer(from_pid);
    if (peer.get() == nullptr) {
        LOG4CXX_INFO(logger_, "from_pid: " << from_pid << " not log in system");
        response(con, "from_pid: " + std::to_string(from_pid) +
                          " not log in system");
        return;
    }
    std::lock_guard<std::mutex> plock(mu_);
    int64_t rid;
    {
        std::lock_guard<std::mutex> rlock(mu_);
        while (rooms_.find(next_id_) != rooms_.end()) next_id_++;
        rid = next_id_++;
        std::shared_ptr<Room> room = std::make_shared<Room>(rid);
        room->addPeer(from_pid, peer);
        rooms_.emplace(rid, room);
    }
    LOG4CXX_INFO(logger_, "Room " << rid << " created by " << from_pid);
    response(con, "success",
             {"type", "createRoom", "rid", std::to_string(rid)});
}

void RoomManager::joinRoom(Type::connection_ptr con, int64_t rid,
                           int64_t from_pid) {
    LOG4CXX_INFO(logger_,
                 "from_pid: " << from_pid << " want to join room: " << rid);
    std::shared_ptr<Peer> peer = PeerManager::getInstance()->getPeer(from_pid);
    if (peer.get() == nullptr) {
        LOG4CXX_INFO(logger_, "from_pid: " << from_pid << " not log in system");
        response(con, "from_pid: " + std::to_string(from_pid) +
                          " not log in system");
        return;
    }
    std::shared_ptr<Room> room = getRoom(rid);
    if (room && room->addPeer(from_pid, peer)) {
        response(con, "success",
                 {"type", "joinRoom", "rid", std::to_string(rid)});
    } else
        response(con, "room not exist!");
}

void RoomManager::leftRoom(Type::connection_ptr con, int64_t rid,
                           int64_t from_pid) {
    LOG4CXX_INFO(logger_,
                 "from_pid: " << from_pid << " want to left room: " << rid);
    std::shared_ptr<Room> room = getRoom(rid);
    if (room && room->removePeer(from_pid)) {
        response(con, "success",
                 {"type", "leftRoom", "rid", std::to_string(rid)});
    } else
        response(con, "room not exist!");
}

void RoomManager::sendToRoom(Type::connection_ptr con, int64_t rid,
                             int64_t from_pid, const std::string& msg) {}

void RoomManager::getPeersInRoom(Type::connection_ptr con, int64_t rid,
                                 int64_t from_pid) {}

void RoomManager::call(Type::connection_ptr con, int64_t rid, int64_t from_pid,
                       int64_t dest_pid) {
    LOG4CXX_INFO(logger_,
                 "from_pid: " << from_pid << " want to call dest " << dest_pid);
    std::shared_ptr<Room> room = getRoom(rid);
    if (room && room->session_.call(from_pid, dest_pid)) {
        response(con, "success",
                 {"type", "call", "dest", std::to_string(dest_pid)});
    } else
        response(con, "call failed");
}

void RoomManager::callAccept(Type::connection_ptr con, int64_t rid,
                             int64_t from_pid, int64_t dest_pid) {
    LOG4CXX_INFO(logger_, "from_pid: " << from_pid << " call accept with dest "
                                       << dest_pid);
    std::shared_ptr<Room> room = getRoom(rid);
    if (!(room && room->session_.callAccept(from_pid, dest_pid))) {
        response(con, "success",
                 {"type", "callAccept", "dest", std::to_string(dest_pid)});
    } else
        response(con, "callAccept failed");
}
void RoomManager::callReject(Type::connection_ptr con, int64_t rid,
                             int64_t from_pid, int64_t dest_pid) {
    LOG4CXX_INFO(logger_, "from_pid: " << from_pid << "call reject with dest "
                                       << dest_pid);
    std::shared_ptr<Room> room = getRoom(rid);
    if (!(room && room->session_.callReject(from_pid, dest_pid))) {
        response(con, "success",
                 {"type", "callReject", "dest", std::to_string(dest_pid)});
    } else
        response(con, "callReject failed");
}

void RoomManager::invite(Type::connection_ptr con, int64_t rid,
                         int64_t from_pid, int64_t dest_pid) {
    LOG4CXX_INFO(logger_, "from_pid: " << from_pid << " want to invite dest "
                                       << dest_pid);
    std::shared_ptr<Room> room = getRoom(rid);
    if (room && room->session_.invite(from_pid, dest_pid)) {
        response(con, "success",
                 {"type", "invite", "dest", std::to_string(dest_pid)});
    } else
        response(con, "invite failed");
}
void RoomManager::inviteAccept(Type::connection_ptr con, int64_t rid,
                               int64_t from_pid, int64_t dest_pid) {
    LOG4CXX_INFO(logger_, "from_pid: " << from_pid
                                       << " invite accept with dest "
                                       << dest_pid);
    std::shared_ptr<Room> room = getRoom(rid);
    if (!(room && room->session_.inviteAccept(from_pid, dest_pid))) {
        response(con, "success",
                 {"type", "inviteAccept", "dest", std::to_string(dest_pid)});
    } else
        response(con, "inviteAccept failed");
}
void RoomManager::inviteReject(Type::connection_ptr con, int64_t rid,
                               int64_t from_pid, int64_t dest_pid) {
    LOG4CXX_INFO(logger_, "from_pid: " << from_pid
                                       << " invite reject with dest "
                                       << dest_pid);
    std::shared_ptr<Room> room = getRoom(rid);
    if (!(room && room->session_.inviteReject(from_pid, dest_pid))) {
        response(con, "success",
                 {"type", "inviteReject", "dest", std::to_string(dest_pid)});
    } else
        response(con, "inviteReject failed");
}

void RoomManager::joinSession(Type::connection_ptr con, int64_t rid,
                              int64_t from_pid) {
    LOG4CXX_INFO(
        logger_,
        "from_pid: " << from_pid << " want to join session in room: " << rid);
    std::shared_ptr<Room> room = getRoom(rid);
    if (room && room->session_.joinSession(from_pid)) {
        response(con, "success",
                 {"type", "joinSession", "rid", std::to_string(rid)});
    } else
        response(con, "join Session failed");
}

void RoomManager::leftSession(Type::connection_ptr con, int64_t rid,
                              int64_t from_pid) {
    LOG4CXX_INFO(
        logger_,
        "from_pid: " << from_pid << " want to left session in room: " << rid);
    std::shared_ptr<Room> room = getRoom(rid);
    if (room && room->session_.leftSession(from_pid)) {
        response(con, "success",
                 {"type", "leftSession", "rid", std::to_string(rid)});
    } else
        response(con, "left Session failed");
}

void RoomManager::getSessionStatus(Type::connection_ptr con, int64_t rid) {
    LOG4CXX_INFO(
        logger_,
        "ip: " << con->get_remote_endpoint()
               << " want to getSessionStatus session in room: " << rid);
    std::shared_ptr<Room> room = getRoom(rid);
    std::string status = room->session_.getSessionStatus();
    con->send(status);
}

void RoomManager::sendToSession(Type::connection_ptr con, int64_t rid,
                                int64_t from_pid, const std::string& msg) {
    LOG4CXX_INFO(
        logger_,
        "from_pid: " << from_pid
                     << " want to send msg to session in room: " << rid);
    std::shared_ptr<Room> room = getRoom(rid);
    if (room && room->session_.sendToSession(from_pid, msg)) {
        response(con, "success",
                 {"type", "sendToSession", "rid", std::to_string(rid)});
    } else
        response(con, "sendToSession failed");
}

void RoomManager::sendSDPOffer(Type::connection_ptr con, int64_t rid,
                               int64_t from_pid, int64_t dest_pid,
                               const std::string& offer) {
    LOG4CXX_INFO(logger_, "from_pid: " << from_pid
                                       << " want to send sdp offer to dest "
                                       << dest_pid);
    std::shared_ptr<Room> room = getRoom(rid);
    if (room && room->session_.sendSDPOffer(from_pid, dest_pid, offer)) {
        response(
            con, "success",
            {"type", "sendSDPOffer", "dest_pid", std::to_string(dest_pid)});
    } else
        response(con, "sendSDPOffer failed");
}

void RoomManager::sendSDPAnswer(Type::connection_ptr con, int64_t rid,
                                int64_t from_pid, int64_t dest_pid,
                                const std::string& answer) {
    LOG4CXX_INFO(logger_, "from_pid: " << from_pid
                                       << " want to send sdp answer to dest "
                                       << dest_pid);
    std::shared_ptr<Room> room = getRoom(rid);
    if (room && room->session_.sendSDPAnswer(from_pid, dest_pid, answer)) {
        response(
            con, "success",
            {"type", "sendSDPAnswer", "dest_pid", std::to_string(dest_pid)});
    } else
        response(con, "sendSDPAnswer failed");
}

void RoomManager::sendICECandidate(Type::connection_ptr con, int64_t rid,
                                   int64_t from_pid, int64_t dest_pid,
                                   const std::string& candidate) {
    LOG4CXX_INFO(logger_, "from_pid: " << from_pid
                                       << " want to send ICECandidate to dest "
                                       << dest_pid);
    std::shared_ptr<Room> room = getRoom(rid);
    if (room && room->session_.sendICECandidate(from_pid, dest_pid, candidate)) {
        response(
            con, "success",
            {"type", "sendICECandidate", "dest_pid", std::to_string(dest_pid)});
    } else
        response(con, "sendICECandidate failed");
}

void RoomManager::connected(Type::connection_ptr con, int64_t rid,
                            int64_t from_pid) {
    LOG4CXX_INFO(logger_,
                 "from_pid: " << from_pid << " success connected." << rid);
    std::shared_ptr<Room> room = getRoom(rid);
    room->session_.connected(from_pid);
}

void RoomManager::openCamera(Type::connection_ptr con, int64_t rid,
                             int64_t from_pid) {
    LOG4CXX_INFO(
        logger_,
        "from_pid: " << from_pid << " open camera in room's session: " << rid);
    std::shared_ptr<Room> room = getRoom(rid);
    room->session_.openCamera(from_pid);
}

void RoomManager::closeCamera(Type::connection_ptr con, int64_t rid,
                              int64_t from_pid) {
    LOG4CXX_INFO(
        logger_,
        "from_pid: " << from_pid << " close camera in room's session: " << rid);
    std::shared_ptr<Room> room = getRoom(rid);
    room->session_.closeCamera(from_pid);
}

void RoomManager::openScreen(Type::connection_ptr con, int64_t rid,
                             int64_t from_pid) {
    LOG4CXX_INFO(
        logger_,
        "from_pid: " << from_pid << " open screen in room's session: " << rid);
    std::shared_ptr<Room> room = getRoom(rid);
    room->session_.openScreen(from_pid);
}

void RoomManager::closeScreen(Type::connection_ptr con, int64_t rid,
                              int64_t from_pid) {
    LOG4CXX_INFO(
        logger_,
        "from_pid: " << from_pid << " close screen in room's session: " << rid);
    std::shared_ptr<Room> room = getRoom(rid);
    room->session_.closeScreen(from_pid);
}

void RoomManager::openAudio(Type::connection_ptr con, int64_t rid,
                            int64_t from_pid) {
    LOG4CXX_INFO(
        logger_,
        "from_pid: " << from_pid << " open audio in room's session: " << rid);
    std::shared_ptr<Room> room = getRoom(rid);
    room->session_.openAudio(from_pid);
}

void RoomManager::closeAudio(Type::connection_ptr con, int64_t rid,
                             int64_t from_pid) {
    LOG4CXX_INFO(
        logger_,
        "from_pid: " << from_pid << " close audio in room's session: " << rid);
    std::shared_ptr<Room> room = getRoom(rid);
    room->session_.closeAudio(from_pid);
}

std::shared_ptr<Room> RoomManager::getRoom(int64_t rid) {
    std::lock_guard<std::mutex> plock(mu_);
    if (rooms_.find(rid) != rooms_.end()) {
        return rooms_.at(rid);
    }
    return {};
}
