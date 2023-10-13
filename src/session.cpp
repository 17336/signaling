#include "session.h"

#include "sessionDumper.h"
#include "util.h"

log4cxx::LoggerPtr Session::logger_ = log4cxx::Logger::getLogger("processor");

Session::Session(std::unordered_map<int64_t, std::shared_ptr<Peer>> *peers,
                 std::mutex *mu, int64_t room_id)
    : peers_(peers),
      mu_(mu),
      id_(room_id),
      count_(0),
      start_time_(),
      end_time_() {}

Session &Session::operator=(const Session &other) {
    this->peers_ = other.peers_;
    this->mu_ = other.mu_;
    this->id_ = other.id_;
    this->count_.store(other.count_.load());
    this->start_time_ = other.start_time_;
    this->end_time_ = other.end_time_;
    return *this;
}

bool Session::call(int64_t from_pid, int64_t dest_pid) {
    if (count_.load() != 0) {
        LOG4CXX_WARN(logger_,
                     "already have user in session, join instead call.");
        return false;
    }
    std::shared_ptr<Peer> from;
    std::shared_ptr<Peer> dest;
    if (!(from = getPeer(from_pid)) || !(dest = getPeer(dest_pid))) {
        LOG4CXX_WARN(logger_, "failed to get peer id: " << from_pid << "or "
                                                        << dest_pid);
        return false;
    }
    return this->sendSignal(from, dest, "call");
}

// todo: 成功后把from、dest加入到session
bool Session::callAccept(int64_t from_pid, int64_t dest_pid) {
    std::shared_ptr<Peer> from;
    std::shared_ptr<Peer> dest;
    if (!(from = getPeer(from_pid)) || !(dest = getPeer(dest_pid))) {
        LOG4CXX_WARN(logger_,
                     "failed to get peer id: " << from_pid << "or " << dest_pid
                                               << ", room id: " << id_);
        return false;
    }
    from->peer_status_.setIsInSession(true);
    dest->peer_status_.setIsInSession(true);
    std::string now = nowTime();
    from->peer_status_.join_time_ = now;
    dest->peer_status_.join_time_ = now;
    if (count_.fetch_add(2) == 0) {
        start_time_ = now;
    }
    return this->sendSignal(from, dest, "callAccept");
}

bool Session::callReject(int64_t from_pid, int64_t dest_pid) {
    std::shared_ptr<Peer> from;
    std::shared_ptr<Peer> dest;
    if (!(from = getPeer(from_pid)) || !(dest = getPeer(dest_pid))) {
        LOG4CXX_WARN(logger_, "failed to get peer id: " << from_pid << "or "
                                                        << dest_pid);
        return false;
    }
    return this->sendSignal(from, dest, "callReject");
}

// check from in session
bool Session::invite(int64_t from_pid, int64_t dest_pid) {
    std::shared_ptr<Peer> from;
    std::shared_ptr<Peer> dest;
    if (!(from = getPeer(from_pid)) || !(dest = getPeer(dest_pid))) {
        LOG4CXX_WARN(logger_, "failed to get peer id: " << from_pid << "or "
                                                        << dest_pid);
        return false;
    }
    return this->sendSignal(from, dest, "invite");
}

// todo
bool Session::inviteAccept(int64_t from_pid, int64_t dest_pid) {
    std::shared_ptr<Peer> from;
    std::shared_ptr<Peer> dest;
    if (!(from = getPeer(from_pid)) || !(dest = getPeer(dest_pid))) {
        LOG4CXX_WARN(logger_, "failed to get peer id: " << from_pid << "or "
                                                        << dest_pid);
        return false;
    }
    from->peer_status_.setIsInSession(true);
    std::string now = nowTime();
    from->peer_status_.join_time_ = now;
    if (count_.fetch_add(1) == 0) {
        start_time_ = now;
    }
    return this->sendSignal(from, dest, "inviteAccept") &&
           this->sendSignal(from, "joinSession");
}

bool Session::inviteReject(int64_t from_pid, int64_t dest_pid) {
    std::shared_ptr<Peer> from;
    std::shared_ptr<Peer> dest;
    if (!(from = getPeer(from_pid)) || !(dest = getPeer(dest_pid))) {
        LOG4CXX_WARN(logger_, "failed to get peer id: " << from_pid << "or "
                                                        << dest_pid);
        return false;
    }
    return this->sendSignal(from, dest, "inviteReject");
}

bool Session::joinSession(int64_t from_pid) {
    std::shared_ptr<Peer> from;
    if (!(from = getPeer(from_pid))) {
        LOG4CXX_WARN(logger_, "not in room peer id: " << from_pid);
        return false;
    }
    std::string now = nowTime();
    from->peer_status_.join_time_ = now;
    if (count_.fetch_add(1) == 0) {
        start_time_ = now;
    }
    from->peer_status_.setIsInSession(true);
    return this->sendSignal(from, "joinSession");
}

bool Session::leftSession(int64_t from_pid) {
    std::shared_ptr<Peer> from;
    if (!(from = getPeer(from_pid))) {
        LOG4CXX_WARN(logger_, "failed to get peer id: " << from_pid);
        return false;
    }
    from->peer_status_.setIsInSession(false);
    LOG4CXX_DEBUG(logger_, "join time" << from->peer_status_.join_time_);
    std::string now = nowTime();
    from->peer_status_.left_time_ = now;
    // todo: here send to sql and reset.
    if (count_.fetch_sub(1) == 1) {
        std::lock_guard<std::mutex> lock(*mu_);
        LOG4CXX_INFO(logger_, "all user left session, will dump");
        end_time_ = nowTime();
        auto dumper = SessionDumper::getInstance();
        SessionLog log;
        log.room_id_ = id_;
        log.start_time_ = start_time_;
        log.end_time_ = end_time_;
        for (auto &p : *peers_) {
            if (p.second->peer_status_.wasInSession()) {
                log.peers.push_back(
                    {p.second->id(), p.second->name(), p.second->ip()});
                log.statuses.push_back(p.second->peer_status_);
            }
            p.second->peer_status_.reset();
            start_time_.clear();
            end_time_.clear();
        }
        dumper->addSessionLog(log);
    }
    return true;
}

void Session::getSessionStatus(rapidjson::Document &d) {
    std::lock_guard<std::mutex> lock(*mu_);
    d.SetObject();
    d.AddMember("rid", id_, d.GetAllocator());
    rapidjson::Value statuses(rapidjson::kArrayType);
    for (auto &p : *peers_) {
        if (p.second->peer_status_.isInSession()) {
            auto &ps = p.second->peer_status_;
            rapidjson::Document status;  // Null
            status.SetObject();
            status.AddMember("pid", p.first, status.GetAllocator());
            status.AddMember("name",
                             rapidjson::Value(p.second->name().c_str(),
                                              status.GetAllocator()),
                             status.GetAllocator());
            status.AddMember(
                "ip",
                rapidjson::Value(p.second->ip().c_str(), status.GetAllocator()),
                status.GetAllocator());
            status.AddMember("AudioUsed", ps.isAudioUsed(),
                             status.GetAllocator());
            status.AddMember("AudioUsing", ps.isAudioUsing(),
                             status.GetAllocator());
            status.AddMember("CameraUsed", ps.isCameraUsed(),
                             status.GetAllocator());
            status.AddMember("CameraUsing", ps.isCameraUsing(),
                             status.GetAllocator());
            status.AddMember("ScreenUsed", ps.isScreenUsed(),
                             status.GetAllocator());
            status.AddMember("ScreenUsing", ps.isScreenUsing(),
                             status.GetAllocator());
            status.AddMember("SendOffer", ps.isSendOffer(),
                             status.GetAllocator());
            status.AddMember("ReceiveOffer", ps.isReceiveOffer(),
                             status.GetAllocator());
            status.AddMember("SendAnswer", ps.isSendAnswer(),
                             status.GetAllocator());
            status.AddMember("ReceiveAnswer", ps.isReceiveAnswer(),
                             status.GetAllocator());
            status.AddMember("SendCandidate", ps.isSendCandidate(),
                             status.GetAllocator());
            status.AddMember("ReceiveCandidate", ps.isReceiveCandidate(),
                             status.GetAllocator());
            status.AddMember("Connected", ps.isConnected(),
                             status.GetAllocator());
            statuses.PushBack(status, d.GetAllocator());
        }
    }
    d.AddMember("statuses", statuses, d.GetAllocator());
}

bool Session::sendToSession(int64_t from_pid, const std::string &msg) {
    std::lock_guard<std::mutex> lock(*mu_);
    if (peers_->find(from_pid) == peers_->end() ||
        !(*peers_)[from_pid]->peer_status_.isInSession()) {
        LOG4CXX_WARN(logger_, "pid: " << from_pid << " not in session " << id_
                                      << ", can not send msg to session.");
        return false;
    }
    int len = peers_->size();
    for (auto p = peers_->begin(); p != peers_->end();) {
        try {
            rapidjson::Document d;
            d.SetObject();
            d.AddMember("type", "text", d.GetAllocator());
            d.AddMember("from", "session", d.GetAllocator());
            d.AddMember("from_pid", from_pid, d.GetAllocator());
            d.AddMember("msg", "text", d.GetAllocator());
            d.AddMember("text", rapidjson::Value(msg.c_str(), d.GetAllocator()),
                        d.GetAllocator());
            p->second->sendMsg(getString(d));
        } catch (std::exception const &e) {
            LOG4CXX_ERROR(logger_, e.what());
            LOG4CXX_ERROR(logger_, "failed to send msg to pid "
                                       << p->first << "in room " << id_);
            p = peers_->erase(p);
            continue;
        }
        ++p;
    }
    return true;
}

bool Session::connected(int64_t from_pid) {
    std::shared_ptr<Peer> from;
    if (!(from = getPeer(from_pid))) {
        LOG4CXX_WARN(logger_, "failed to get peer id: " << from_pid);
        return false;
    }
    from->peer_status_.setConnected(true);
    return true;
}

bool Session::openCamera(int64_t from_pid) {
    std::shared_ptr<Peer> from;
    if (!(from = getPeer(from_pid))) {
        LOG4CXX_WARN(logger_, "failed to get peer id: " << from_pid);
        return false;
    }
    from->peer_status_.setCameraUsing(true);
    return this->sendSignal(from, "openCamera");
}

bool Session::closeCamera(int64_t from_pid) {
    std::shared_ptr<Peer> from;
    if (!(from = getPeer(from_pid))) {
        LOG4CXX_WARN(logger_, "failed to get peer id: " << from_pid);
        return false;
    }
    from->peer_status_.setCameraUsing(false);
    return this->sendSignal(from, "closeCamera");
}

bool Session::openScreen(int64_t from_pid) {
    std::shared_ptr<Peer> from;
    if (!(from = getPeer(from_pid))) {
        LOG4CXX_WARN(logger_, "failed to get peer id: " << from_pid);
        return false;
    }
    from->peer_status_.setScreenUsing(true);
    return this->sendSignal(from, "openScreen");
}

bool Session::closeScreen(int64_t from_pid) {
    std::shared_ptr<Peer> from;
    if (!(from = getPeer(from_pid))) {
        LOG4CXX_WARN(logger_, "failed to get peer id: " << from_pid);
        return false;
    }
    from->peer_status_.setScreenUsing(false);
    return this->sendSignal(from, "closeScreen");
}

bool Session::openAudio(int64_t from_pid) {
    std::shared_ptr<Peer> from;
    if (!(from = getPeer(from_pid))) {
        LOG4CXX_WARN(logger_, "failed to get peer id: " << from_pid);
        return false;
    }
    from->peer_status_.setAudioUsing(true);
    return this->sendSignal(from, "openAudio");
}

bool Session::closeAudio(int64_t from_pid) {
    std::shared_ptr<Peer> from;
    if (!(from = getPeer(from_pid))) {
        LOG4CXX_WARN(logger_, "failed to get peer id: " << from_pid);
        return false;
    }
    from->peer_status_.setAudioUsing(false);
    return this->sendSignal(from, "closeAudio");
}

// 会话协商
bool Session::sendSDPOffer(int64_t from_pid, int64_t dest_pid,
                           const std::string &offer) {
    std::shared_ptr<Peer> from;
    std::shared_ptr<Peer> dest;
    if (!(from = getPeer(from_pid)) || !(dest = getPeer(dest_pid))) {
        LOG4CXX_WARN(logger_, "failed to get peer id: " << from_pid << "or "
                                                        << dest_pid);
        return false;
    }

    if (this->sendSignal(from, dest, "SDPOffer", {"offer", offer})) {
        from->peer_status_.setSendOffer(true);
        dest->peer_status_.setReceiveOffer(true);
        return true;
    }
    return false;
}

bool Session::sendSDPAnswer(int64_t from_pid, int64_t dest_pid,
                            const std::string &answer) {
    std::shared_ptr<Peer> from;
    std::shared_ptr<Peer> dest;
    if (!(from = getPeer(from_pid)) || !(dest = getPeer(dest_pid))) {
        LOG4CXX_WARN(logger_, "failed to get peer id: " << from_pid << "or "
                                                        << dest_pid);
        return false;
    }

    if (this->sendSignal(from, dest, "SDPAnswer", {"answer", answer})) {
        from->peer_status_.setSendAnswer(true);
        dest->peer_status_.setReceiveAnswer(true);
        return true;
    }
    return false;
}
bool Session::sendICECandidate(int64_t from_pid, int64_t dest_pid,
                               const std::string &candidate) {
    std::shared_ptr<Peer> from;
    std::shared_ptr<Peer> dest;
    if (!(from = getPeer(from_pid)) || !(dest = getPeer(dest_pid))) {
        LOG4CXX_WARN(logger_, "failed to get peer id: " << from_pid << "or "
                                                        << dest_pid);
        return false;
    }

    if (this->sendSignal(from, dest, "ICECandidate",
                         {"candidate", candidate})) {
        from->peer_status_.setSendCandidate(true);
        dest->peer_status_.setReceiveCandidate(true);
        return true;
    }
    return false;
}

std::shared_ptr<Peer> Session::getPeer(int64_t pid) {
    std::lock_guard<std::mutex> lock(*mu_);
    if (peers_->find(pid) != peers_->end()) {
        return peers_->at(pid);
    }
    return {};
}

bool Session::sendSignal(std::shared_ptr<Peer> &from,
                         std::shared_ptr<Peer> &dest, const std::string &type,
                         const std::vector<std::string> &kvs) {
    rapidjson::Document d;  // Null
    d.SetObject();
    d.AddMember("type", rapidjson::Value(type.c_str(), d.GetAllocator()),
                d.GetAllocator());
    d.AddMember("msg", "signaling", d.GetAllocator());
    d.AddMember("from_pid", from->id(), d.GetAllocator());
    for (int i = 1; i < kvs.size(); i += 2) {
        d.AddMember(rapidjson::Value(kvs[i - 1].c_str(), d.GetAllocator()),
                    rapidjson::Value(kvs[i].c_str(), d.GetAllocator()),
                    d.GetAllocator());
    }
    try {
        dest->sendMsg(getString(d));
    } catch (std::exception const &e) {
        LOG4CXX_ERROR(logger_,
                      "erase pid: " << from->id() << ", because " << e.what());
        std::lock_guard<std::mutex> lock(*mu_);
        peers_->erase(dest->id());
    }
    return true;
}

bool Session::sendSignal(std::shared_ptr<Peer> &peer, const std::string &type,
                         const std::vector<std::string> &kvs) {
    rapidjson::Document d;  // Null
    d.SetObject();
    d.AddMember("type", rapidjson::Value(type.c_str(), d.GetAllocator()),
                d.GetAllocator());
    d.AddMember("msg", "signaling", d.GetAllocator());
    d.AddMember("from_pid", peer->id(), d.GetAllocator());
    for (int i = 1; i < kvs.size(); i += 2) {
        d.AddMember(rapidjson::Value(kvs[i - 1].c_str(), d.GetAllocator()),
                    rapidjson::Value(kvs[i].c_str(), d.GetAllocator()),
                    d.GetAllocator());
    }
    std::lock_guard<std::mutex> lock(*mu_);
    for (auto p = peers_->begin(); p != peers_->end();) {
        if (p->second->peer_status_.isInSession() && p->first != peer->id()) {
            try {
                p->second->sendMsg(getString(d));
            } catch (std::exception const &e) {
                LOG4CXX_ERROR(logger_, "erase pid: " << p->second->id()
                                                     << ", because "
                                                     << e.what());
                p = peers_->erase(p);
                continue;
            }
        }
        ++p;
    }

    return true;
}
