#ifndef _ROOMMANAGER_H_
#define _ROOMMANAGER_H_

#include <mutex>
#include <unordered_map>

#include "room.h"
#include "session.h"
#include "type.h"

class RoomManager {
public:
    static RoomManager* getInstance();
    RoomManager(const RoomManager&) = delete;
    RoomManager& operator=(const RoomManager&) = delete;

    void searchRoom(Type::connection_ptr con, int64_t rid, int64_t from_pid);
    void createRoom(Type::connection_ptr con, int64_t from_pid);
    void joinRoom(Type::connection_ptr con, int64_t rid, int64_t from_pid);
    void leftRoom(Type::connection_ptr con, int64_t rid, int64_t from_pid);
    // 给room发消息除了from_pid
    void sendToRoom(Type::connection_ptr con, int64_t rid, int64_t from_pid,
                    const std::string& msg);
    void getPeersInRoom(Type::connection_ptr con, int64_t rid,
                        int64_t from_pid);

    // 会话接口
    void call(Type::connection_ptr con, int64_t rid, int64_t from_pid,
              int64_t dest_pid);
    void callAccept(Type::connection_ptr con, int64_t rid, int64_t from_pid,
                    int64_t dest_pid);
    void callReject(Type::connection_ptr con, int64_t rid, int64_t from_pid,
                    int64_t dest_pid);

    void invite(Type::connection_ptr con, int64_t rid, int64_t from_pid,
                int64_t dest_pid);
    void inviteAccept(Type::connection_ptr con, int64_t rid, int64_t from_pid,
                      int64_t dest_pid);
    void inviteReject(Type::connection_ptr con, int64_t rid, int64_t from_pid,
                      int64_t dest_pid);

    void joinSession(Type::connection_ptr con, int64_t rid, int64_t from_pid);
    void leftSession(Type::connection_ptr con, int64_t rid, int64_t from_pid);

    void getSessionStatus(Type::connection_ptr con, int64_t rid);

    void sendToSession(Type::connection_ptr con, int64_t rid, int64_t from_pid,
                       const std::string& msg);

    // 会话协商
    // todo:现在是发给个人的，muc架构应该是发给turn的
    void sendSDPOffer(Type::connection_ptr con, int64_t rid, int64_t from_pid,
                      int64_t dest_pid, const std::string& offer);

    void sendSDPAnswer(Type::connection_ptr con, int64_t rid, int64_t from_pid,
                       int64_t dest_pid, const std::string& answer);
    void sendICECandidate(Type::connection_ptr con, int64_t rid,
                          int64_t from_pid, int64_t dest_pid,
                          const std::string& candidate);
    void connected(Type::connection_ptr con, int64_t rid, int64_t from_pid);

    // 会话中控制
    void openCamera(Type::connection_ptr con, int64_t rid, int64_t from_pid);
    void closeCamera(Type::connection_ptr con, int64_t rid, int64_t from_pid);

    void openScreen(Type::connection_ptr con, int64_t rid, int64_t from_pid);
    void closeScreen(Type::connection_ptr con, int64_t rid, int64_t from_pid);

    void openAudio(Type::connection_ptr con, int64_t rid, int64_t from_pid);
    void closeAudio(Type::connection_ptr con, int64_t rid, int64_t from_pid);

private:
    static log4cxx::LoggerPtr logger_;
    RoomManager();

    std::shared_ptr<Room> getRoom(int64_t rid);
    
    std::unordered_map<int64_t, std::shared_ptr<Room>> rooms_;
    std::mutex mu_;
    int64_t next_id_;
};

#endif  // _ROOMMANAGER_H_