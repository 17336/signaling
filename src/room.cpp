#include "room.h"

#include "session.h"

log4cxx::LoggerPtr Room::logger_ = log4cxx::Logger::getRootLogger();

Room::Room(int64_t id) : id_(id), peers_(), mu_(), session_(&peers_, &mu_, id) {}

Room::Room(const Room& other)
    : id_(other.id_), peers_(other.peers_), mu_(), session_(&peers_, &mu_, other.id_) {}

Room::Room(Room&& other)
    : id_(other.id_), peers_(), mu_(), session_(&peers_, &mu_, other.id_) {
    this->id_ = other.id_;
    this->peers_.swap(other.peers_);
}

Room& Room::operator=(const Room& other) {
    this->id_ = other.id_;
    this->peers_ = other.peers_;
    this->session_ = Session(&peers_, &mu_, id_);
    return *this;
}

Room::~Room() {}

bool Room::addPeer(int64_t pid, std::shared_ptr<Peer> peer) {
    std::lock_guard<std::mutex> lock(mu_);
    if (peers_.find(pid) != peers_.end()) {
        LOG4CXX_WARN(logger_, pid << " already in Room");
    } else {
        peers_.emplace(pid, peer);
    }
    return true;
}

bool Room::removePeer(int64_t pid) {
    std::lock_guard<std::mutex> lock(mu_);
    if (peers_.find(pid) == peers_.end()) {
        LOG4CXX_WARN(logger_, pid << " not in Room");
    } else {
        peers_.erase(pid);
    }
    return true;
}

bool Room::sendToRoom(int64_t from_pid, const std::string& msg) {
    std::lock_guard<std::mutex> lock(mu_);
    if (peers_.find(from_pid) == peers_.end()) {
        LOG4CXX_WARN(logger_, "pid: " << from_pid << " not in room " << id_
                                      << ", can not send msg to room.");
        return false;
    }
    int len = peers_.size();
    for (auto p = peers_.begin(); p != peers_.end();) {
        try {
            p->second->sendMsg(msg);
        } catch (std::exception const& e) {
            LOG4CXX_ERROR(logger_, e.what());
            LOG4CXX_ERROR(logger_, "failed to send msg to pid "
                                       << p->first << "in room " << id_);
            p = peers_.erase(p);
            continue;
        }
        ++p;
    }
    return true;
};

bool Room::isInroom(int64_t from_pid) {
    std::lock_guard<std::mutex> lock(mu_);
    return peers_.find(from_pid) != peers_.end();
}

bool Room::empty() {
    std::lock_guard<std::mutex> lock(mu_);
    return peers_.empty();
}