#ifndef _WORKERPOOL_H_
#define _WORKERPOOL_H_

#include <atomic>
#include <thread>
#include <vector>

#include "context.h"
#include "log4cxx/log4cxx.h"
#include "log4cxx/logger.h"
#include "peerManager.h"
#include "producerConsumerQueue.h"
#include "rapidjson/document.h"
#include "roomManager.h"
#include "type.h"

class WorkerPool {
public:

    WorkerPool();
    ~WorkerPool();
    void addContext(const Context &context);
    void init();
    void start();
    void stop();

private:
    void run();
    void process(Context &context);
    inline bool getFromPid(Type::connection_ptr con, rapidjson::Document &doc,
                           int64_t *from_pid, bool sendError = true);
    inline bool getDestPid(Type::connection_ptr con, rapidjson::Document &doc,
                           int64_t *dest_pid, bool sendError = true);
    inline bool getRid(Type::connection_ptr con, rapidjson::Document &doc,
                       int64_t *rid, bool sendError = true);
    inline bool getName(Type::connection_ptr con, rapidjson::Document &doc,
                        std::string *name, bool sendError = true);
    inline bool getMsg(Type::connection_ptr con, rapidjson::Document &doc,
                       std::string *msg, bool sendError = true);

    RoomManager *room_manager_;
    PeerManager *peer_manager_;
    int count_;
    std::atomic_bool start_;
    std::vector<std::thread> threads_;
    static log4cxx::LoggerPtr logger_;
    ProducerConsumerQueue<Context> input_;
};

#endif  // _WORKERPOOL_H_