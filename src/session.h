#ifndef _SESSION_H_
#define _SESSION_H_

#include <memory>
#include <mutex>
#include <unordered_map>

#include "log4cxx/log4cxx.h"
#include "log4cxx/logger.h"
#include "peer.h"
#include "sessionInterface.h"

class Session : public SessionNegotiate, public SessionControl {
public:
    Session() = delete;
    Session(std::unordered_map<int64_t, std::shared_ptr<Peer>> *peers,
            std::mutex *mu, int64_t room_id);
    Session &operator=(const Session &other);

    // 会话成员管理
    bool call(int64_t from_pid, int64_t dest_pid);
    bool callAccept(int64_t from_pid, int64_t dest_pid);
    bool callReject(int64_t from_pid, int64_t dest_pid);

    bool invite(int64_t from_pid, int64_t dest_pid);
    bool inviteAccept(int64_t from_pid, int64_t dest_pid);
    bool inviteReject(int64_t from_pid, int64_t dest_pid);

    bool joinSession(int64_t from_pid);
    bool leftSession(int64_t from_pid);

    std::string getSessionStatus();

    bool sendToSession(int64_t from_pid, const std::string &msg);

    // 会话协商
    bool sendSDPOffer(int64_t from_pid, int64_t dest_pid,
                      const std::string &offer);

    bool sendSDPAnswer(int64_t from_pid, int64_t dest_pid,
                       const std::string &answer);
    bool sendICECandidate(int64_t from_pid, int64_t dest_pid,
                          const std::string &candidate);
    bool connected(int64_t from_pid);

    // 会话控制
    bool openCamera(int64_t from_pid);
    bool closeCamera(int64_t from_pid);
    bool openScreen(int64_t from_pid);
    bool closeScreen(int64_t from_pid);
    bool openAudio(int64_t from_pid);
    bool closeAudio(int64_t from_pid);

private:
    std::shared_ptr<Peer> getPeer(int64_t pid);
    bool sendSignal(std::shared_ptr<Peer> &from, std::shared_ptr<Peer> &dest,
                    const std::string &type,
                    const std::vector<std::string> &kvs = {});
    bool sendSignal(std::shared_ptr<Peer> &peer, const std::string &type,
                    const std::vector<std::string> &kvs = {});

    std::unordered_map<int64_t, std::shared_ptr<Peer>> *peers_;
    std::mutex *mu_;
    int64_t id_;
    static log4cxx::LoggerPtr logger_;
};

#endif  // _SESSION_H_