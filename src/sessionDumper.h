#ifndef _SESSIONDUMPER_H_
#define _SESSIONDUMPER_H_

#include <atomic>
#include <thread>
#include <vector>

#include "connectionPool.h"
#include "peerStatus.h"
#include "producerConsumerQueue.h"

struct PeerInfo {
    int64_t id_;
    std::string name_;
    std::string ip_;
};

struct SessionLog {
    std::vector<PeerStatus> statuses;
    std::vector<PeerInfo> peers;
    std::string start_time_;
    std::string end_time_;
    int64_t room_id_;
};

class SessionDumper {
public:
    static SessionDumper *getInstance();
    void addSessionLog(const SessionLog &);

    ~SessionDumper();

private:
    SessionDumper();
    void run();
    void dump(SessionLog &);

    ProducerConsumerQueue<SessionLog> input_;
    std::thread t_;
    std::atomic<bool> start_;
    static log4cxx::LoggerPtr logger_;
};

#endif  // _SESSIONDUMP_H_