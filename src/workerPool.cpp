#include "workerPool.h"

#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"
#include "util.h"
#include "operate.h"

log4cxx::LoggerPtr WorkerPool::logger_ = log4cxx::Logger::getLogger("server");

WorkerPool::WorkerPool():count_(8) {
    room_manager_ = RoomManager::getInstance();
    peer_manager_ = PeerManager::getInstance();
}

WorkerPool::~WorkerPool() { stop(); }

void WorkerPool::init() {}

void WorkerPool::start() {
    start_ = true;
    for (int i = 0; i < count_; i++) {
        threads_.push_back(std::thread(&WorkerPool::run, this));
    }
    LOG4CXX_INFO(logger_, "start " << count_ << " worker");
}

void WorkerPool::stop() {
    start_ = false;
    for (int i = 0; i < count_; i++) {
        threads_[i].join();
    }
    LOG4CXX_INFO(logger_, "stop " << count_ << " worker");
}

void WorkerPool::addContext(const Context &context) { input_.push(context); }

void WorkerPool::run() {
    Context context;
    while (start_) {
        if (!input_.get(&context, 30000)) {
            LOG4CXX_INFO(logger_, "No context in input");
            continue;
        }
        try {
            process(context);
        } catch (const std::exception &e) {
            LOG4CXX_ERROR(logger_,
                          "failed to process context because:" << e.what());
        }
    }
}

inline bool WorkerPool::getFromPid(Type::connection_ptr con,
                                   rapidjson::Document &doc, int64_t *from_pid,
                                   bool sendError) {
    if (doc.HasMember("from_pid") && doc["from_pid"].IsInt()) {
        *from_pid = doc["from_pid"].GetInt64();
        return true;
    } else if (sendError) {
        response(con, "miss from pid");
    }
    return false;
}

inline bool WorkerPool::getDestPid(Type::connection_ptr con,
                                   rapidjson::Document &doc, int64_t *dest_pid,
                                   bool sendError) {
    if (doc.HasMember("dest_pid") && doc["dest_pid"].IsInt()) {
        *dest_pid = doc["dest_pid"].GetInt64();
        return true;
    } else if (sendError) {
        response(con, "miss dest pid");
    }
    return false;
}

inline bool WorkerPool::getRid(Type::connection_ptr con,
                               rapidjson::Document &doc, int64_t *rid,
                               bool sendError) {
    if (doc.HasMember("rid") && doc["rid"].IsInt()) {
        *rid = doc["rid"].GetInt64();
        return true;
    } else if (sendError) {
        response(con, "miss rid");
    }
    return false;
}

inline bool WorkerPool::getName(Type::connection_ptr con,
                                rapidjson::Document &doc, std::string *name,
                                bool sendError) {
    if (doc.HasMember("name") && doc["name"].IsString() && !(*name = doc["name"].GetString()).empty()) {
        return true;
    } else if (sendError) {
        response(con, "please provide your name!");
    }
    return false;
}

inline bool WorkerPool::getMsg(Type::connection_ptr con,
                               rapidjson::Document &doc, std::string *msg,
                               bool sendError) {
    if (doc.HasMember("msg") && doc["msg"].IsString()) {
        *msg = doc["msg"].GetString();
        return true;
    } else if (sendError) {
        response(con, "please provide your msg!");
    }
    return false;
}

// here
void WorkerPool::process(Context &context) {
    const Type::message_ptr &msg_ptr = context.msg_;
    Type::connection_ptr &con = context.con_;

    // 解析json
    rapidjson::Document doc;
    // payload:{"operate":xxx,"body":xxx, ...}
    std::string payload = msg_ptr->get_payload();
    doc.Parse(payload.c_str());
    if (doc.HasParseError() || !doc.IsObject()) {
        response(con, "Only json format data is supported!");
        return;
    }
    if (!doc.HasMember("operate") || !doc["operate"].IsInt()) {
        response(con, "please support right operate!");
        return;
    }
    // 操作类型，必选
    int opt = doc["operate"].GetInt();

    int64_t from_pid;
    int64_t dest_pid;
    int64_t rid;
    std::string name;
    std::string msg;

    switch (opt) {
        // peer
        case OPERATE::LOG_IN: {
            if (!getName(con, doc, &name))
                return;
            if (getFromPid(con, doc, &from_pid, false)) {
                peer_manager_->logIn(con, from_pid, name);
            } else
                peer_manager_->logIn(con, name);
            break;
        }
        case OPERATE::LOG_OUT:
            if (getFromPid(con, doc, &from_pid)) {
                peer_manager_->logOut(con, from_pid);
            }
            break;
        case OPERATE::SEARCH_PEER:
            if (!getFromPid(con, doc, &from_pid)) {
                return;
            }
            if (getDestPid(con, doc, &dest_pid, false)) {
                peer_manager_->searchPeer(con, from_pid, dest_pid);
            } else if (getName(con, doc, &name, false)) {
                peer_manager_->searchPeer(con, from_pid, name);
            } else {
                response(con, "support dest_pid or name");
            }
            break;
        case OPERATE::SEND_TO:
            if (getFromPid(con, doc, &from_pid) &&
                getDestPid(con, doc, &dest_pid) && getMsg(con, doc, &msg))
                peer_manager_->sendTo(con, from_pid, dest_pid, msg);
            break;

        // room
        case OPERATE::SEARCH_ROOM:
            if (getFromPid(con, doc, &from_pid) && getRid(con, doc, &rid))
                room_manager_->searchRoom(con, rid, from_pid);
            break;
        case OPERATE::CREATE_ROOM:
            if (getFromPid(con, doc, &from_pid))
                room_manager_->createRoom(con, from_pid);
            break;
        case OPERATE::JOIN_ROOM:
            if (getFromPid(con, doc, &from_pid) && getRid(con, doc, &rid))
                room_manager_->joinRoom(con, rid, from_pid);
            break;
        case OPERATE::LEFT_ROOM:
            if (getFromPid(con, doc, &from_pid) && getRid(con, doc, &rid))
                room_manager_->leftRoom(con, rid, from_pid);
            break;
        case OPERATE::SEND_TO_ROOM:
            if (getFromPid(con, doc, &from_pid) && getRid(con, doc, &rid) &&
                getMsg(con, doc, &msg))
                room_manager_->sendToRoom(con, rid, from_pid, msg);
            break;
        case OPERATE::GET_PEERS_IN_ROOM:
            if (getFromPid(con, doc, &from_pid) && getRid(con, doc, &rid))
                room_manager_->getPeersInRoom(con, rid, from_pid);
            break;
        case OPERATE::GET_ALL_PEERS: {
            if (getFromPid(con, doc, &from_pid))
                room_manager_->getAllPeers(con, from_pid);
            break;
        }

        // session
        case OPERATE::CALL:
            if (getFromPid(con, doc, &from_pid) && getRid(con, doc, &rid) &&
                getDestPid(con, doc, &dest_pid))
                room_manager_->call(con, rid, from_pid, dest_pid);
            break;
        case OPERATE::CALL_ACCEPT:
            if (getFromPid(con, doc, &from_pid) && getRid(con, doc, &rid) &&
                getDestPid(con, doc, &dest_pid))
                room_manager_->callAccept(con, rid, from_pid, dest_pid);
            break;
        case OPERATE::CALL_REJECT:
            if (getFromPid(con, doc, &from_pid) && getRid(con, doc, &rid) &&
                getDestPid(con, doc, &dest_pid))
                room_manager_->callReject(con, rid, from_pid, dest_pid);
            break;
        case OPERATE::INVITE:
            if (getFromPid(con, doc, &from_pid) && getRid(con, doc, &rid) &&
                getDestPid(con, doc, &dest_pid))
                room_manager_->invite(con, rid, from_pid, dest_pid);
            break;
        case OPERATE::INVITE_ACCEPT:
            if (getFromPid(con, doc, &from_pid) && getRid(con, doc, &rid) &&
                getDestPid(con, doc, &dest_pid))
                room_manager_->inviteAccept(con, rid, from_pid, dest_pid);
            break;
        case OPERATE::INVITE_REJECT:
            if (getFromPid(con, doc, &from_pid) && getRid(con, doc, &rid) &&
                getDestPid(con, doc, &dest_pid))
                room_manager_->inviteReject(con, rid, from_pid, dest_pid);
            break;
        case OPERATE::JOIN_SESSION:
            if (getFromPid(con, doc, &from_pid) && getRid(con, doc, &rid))
                room_manager_->joinSession(con, rid, from_pid);
            break;
        case OPERATE::LEFT_SESSION:
            if (getFromPid(con, doc, &from_pid) && getRid(con, doc, &rid))
                room_manager_->leftSession(con, rid, from_pid);
            break;
        case OPERATE::GET_SESSION_STATUS:
            if (getFromPid(con, doc, &from_pid) && getRid(con, doc, &rid))
                room_manager_->getSessionStatus(con, rid);
            break;
        case OPERATE::SEND_TO_SESSION:
            if (getFromPid(con, doc, &from_pid) && getRid(con, doc, &rid) &&
                getMsg(con, doc, &msg))
                room_manager_->sendToSession(con, rid, from_pid, msg);
            break;

        // 会话协商
        case OPERATE::SEND_SDP_OFFER: {
            std::string offer;
            if (doc.HasMember("offer") && doc["offer"].IsString()) {
                offer = doc["offer"].GetString();
            } else {
                response(con, "please provide your sdp offer!");
                return;
            }
            if (getFromPid(con, doc, &from_pid) && getRid(con, doc, &rid) &&
                getDestPid(con, doc, &dest_pid))
                room_manager_->sendSDPOffer(con, rid, from_pid, dest_pid,
                                            offer);
            break;
        }
        case OPERATE::SEND_SDP_ANSWER: {
            std::string answer;
            if (doc.HasMember("answer") && doc["answer"].IsString()) {
                answer = doc["answer"].GetString();
            } else {
                response(con, "please provide your sdp answer!");
                return;
            }
            if (getFromPid(con, doc, &from_pid) && getRid(con, doc, &rid) &&
                getDestPid(con, doc, &dest_pid))
                room_manager_->sendSDPAnswer(con, rid, from_pid, dest_pid,
                                             answer);
            break;
        }

        case OPERATE::SEND_ICE_CANDIDATE: {
            std::string candidate;
            if (doc.HasMember("candidate") && doc["candidate"].IsString()) {
                candidate = doc["candidate"].GetString();
            } else {
                response(con, "please provide your candidate!");
                return;
            }
            if (getFromPid(con, doc, &from_pid) && getRid(con, doc, &rid) &&
                getDestPid(con, doc, &dest_pid))
                room_manager_->sendICECandidate(con, rid, from_pid, dest_pid,
                                                candidate);
            break;
        }
        case OPERATE::CONNECTED: {
            if (getFromPid(con, doc, &from_pid) && getRid(con, doc, &rid))
                room_manager_->connected(con, rid, from_pid);
            break;
        }

        // 信令控制
        case OPERATE::OPEN_CAMERA: {
            if (getFromPid(con, doc, &from_pid) && getRid(con, doc, &rid))
                room_manager_->openCamera(con, rid, from_pid);
            break;
        }
        case OPERATE::CLOSE_CAMERA: {
            if (getFromPid(con, doc, &from_pid) && getRid(con, doc, &rid))
                room_manager_->closeCamera(con, rid, from_pid);
            break;
        }
        case OPERATE::OPEN_AUDIO: {
            if (getFromPid(con, doc, &from_pid) && getRid(con, doc, &rid))
                room_manager_->openAudio(con, rid, from_pid);
            break;
        }
        case OPERATE::CLOSE_AUDIO: {
            if (getFromPid(con, doc, &from_pid) && getRid(con, doc, &rid))
                room_manager_->closeAudio(con, rid, from_pid);
            break;
        }
        case OPERATE::OPEN_SCREEN: {
            if (getFromPid(con, doc, &from_pid) && getRid(con, doc, &rid))
                room_manager_->openScreen(con, rid, from_pid);
            break;
        }
        case OPERATE::CLOSE_SCREEN: {
            if (getFromPid(con, doc, &from_pid) && getRid(con, doc, &rid))
                room_manager_->closeScreen(con, rid, from_pid);
            break;
        }
        default:
            response(con, "operate not support");
    }
}
