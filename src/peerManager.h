#ifndef _PEERMANAGER_H_
#define _PEERMANAGER_H_

#include <memory>
#include <mutex>
#include <unordered_map>

#include "log4cxx/log4cxx.h"
#include "log4cxx/logger.h"
#include "peer.h"
#include "type.h"

class PeerManager {
public:
    static PeerManager* getInstance();
    PeerManager(const PeerManager&) = delete;
    PeerManager& operator=(const PeerManager&) = delete;

    ~PeerManager();

    // 用户注册id
    void logIn(Type::connection_ptr con, const std::string& name);
    void logIn(Type::connection_ptr con, int64_t from_pid,
               const std::string& name);
    void logOut(Type::connection_ptr con, int64_t from_pid);
    void searchPeer(Type::connection_ptr con, int64_t from_pid,
                    int64_t dest_pid);
    void searchPeer(Type::connection_ptr con, int64_t from_pid,
                    const std::string& name);
    // 给pid发消息
    void sendTo(Type::connection_ptr con, int64_t from_pid, int64_t dest_pid,
                const std::string& msg);
    std::shared_ptr<Peer> getPeer(int64_t pid);
private:
    PeerManager();

    std::unordered_map<int64_t, std::shared_ptr<Peer>> peers_;
    std::mutex mu_;
    int64_t next_id_;
    static log4cxx::LoggerPtr logger_;
};

#endif  // _PEERMANAGER_H_