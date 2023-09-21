#include "session.h"

#include "rapidjson/document.h"
#include "util.h"

log4cxx::LoggerPtr Session::logger_ = log4cxx::Logger::getRootLogger();

Session::Session(std::unordered_map<int64_t, std::shared_ptr<Peer>> *peers,
                 std::mutex *mu, int64_t room_id)
    : peers_(peers), mu_(mu), id_(room_id) {}

Session &Session::operator=(const Session &other) {
    this->peers_ = other.peers_;
    this->mu_ = other.mu_;
    this->id_ = other.id_;
    return *this;
}

bool Session::call(int64_t from_pid, int64_t dest_pid) {
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
    return this->sendSignal(from, dest, "inviteAccept");
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
        LOG4CXX_WARN(logger_, "failed to get peer id: " << from_pid);
        return false;
    }
    from->peer_status_.setIsInSession(true);
    return true;
}

bool Session::leftSession(int64_t from_pid) {
    std::shared_ptr<Peer> from;
    if (!(from = getPeer(from_pid))) {
        LOG4CXX_WARN(logger_, "failed to get peer id: " << from_pid);
        return false;
    }
    from->peer_status_.setIsInSession(false);
    std::lock_guard<std::mutex> lock(*mu_);
    for (auto &p : *peers_) {
        if (p.second->peer_status_.isInSession()) {
            return true;
        }
    }
    // todo: here send to sql and reset.

    return true;
}

std::string Session::getSessionStatus() {
    std::lock_guard<std::mutex> lock(*mu_);
    rapidjson::Document d;  // Null
    d.SetObject();
    d.AddMember("room_id", id_, d.GetAllocator());
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
    return getString(d);
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
            p->second->sendMsg(msg);
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
    return true;
}

bool Session::closeCamera(int64_t from_pid) {
    std::shared_ptr<Peer> from;
    if (!(from = getPeer(from_pid))) {
        LOG4CXX_WARN(logger_, "failed to get peer id: " << from_pid);
        return false;
    }
    from->peer_status_.setCameraUsing(false);
    return true;
}

bool Session::openScreen(int64_t from_pid) {
    std::shared_ptr<Peer> from;
    if (!(from = getPeer(from_pid))) {
        LOG4CXX_WARN(logger_, "failed to get peer id: " << from_pid);
        return false;
    }
    from->peer_status_.setScreenUsing(true);
    return true;
}

bool Session::closeScreen(int64_t from_pid) {
    std::shared_ptr<Peer> from;
    if (!(from = getPeer(from_pid))) {
        LOG4CXX_WARN(logger_, "failed to get peer id: " << from_pid);
        return false;
    }
    from->peer_status_.setScreenUsing(false);
    return true;
}

bool Session::openAudio(int64_t from_pid) {
    std::shared_ptr<Peer> from;
    if (!(from = getPeer(from_pid))) {
        LOG4CXX_WARN(logger_, "failed to get peer id: " << from_pid);
        return false;
    }
    from->peer_status_.setAudioUsing(true);
    return true;
}

bool Session::closeAudio(int64_t from_pid) {
    std::shared_ptr<Peer> from;
    if (!(from = getPeer(from_pid))) {
        LOG4CXX_WARN(logger_, "failed to get peer id: " << from_pid);
        return false;
    }
    from->peer_status_.setAudioUsing(false);
    return true;
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
    for (int i = 1; i < kvs.size(); i += 2) {
        d.AddMember(rapidjson::Value(kvs[i - 1].c_str(), d.GetAllocator()),
                    rapidjson::Value(kvs[i].c_str(), d.GetAllocator()),
                    d.GetAllocator());
    }
    try {
        peer->sendMsg(getString(d));
    } catch (std::exception const &e) {
        LOG4CXX_ERROR(logger_,
                      "erase pid: " << peer->id() << ", because " << e.what());
        std::lock_guard<std::mutex> lock(*mu_);
        peers_->erase(peer->id());
    }
    return true;
}
