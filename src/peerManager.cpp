#include "peerManager.h"

#include "util.h"

log4cxx::LoggerPtr PeerManager::logger_ = log4cxx::Logger::getRootLogger();

PeerManager* PeerManager::getInstance() {
    static PeerManager peer_manager;
    return &peer_manager;
}

PeerManager::PeerManager() : next_id_(0) {}

PeerManager::~PeerManager() {}

void PeerManager::logIn(Type::connection_ptr con, const std::string& name) {
    LOG4CXX_INFO(logger_, "name: " << name << " which from "
                                   << con->get_remote_endpoint()
                                   << " want to login system");
    int64_t pid;
    {
        std::lock_guard<std::mutex> lock(mu_);
        while (peers_.find(next_id_) != peers_.end()) next_id_++;
        pid = next_id_++;
        peers_.emplace(pid, std::make_shared<Peer>(pid, con, name));
    }
    LOG4CXX_INFO(logger_, "pid:" << pid << " success log in.");
    response(con, "success", {"pid", std::to_string(pid), "type", "log in"});
}

void PeerManager::logIn(Type::connection_ptr con, int64_t from_pid,
                        const std::string& name) {
    LOG4CXX_INFO(logger_, "name: " << name << " which from "
                                   << con->get_remote_endpoint()
                                   << " want to login system");
    {
        std::lock_guard<std::mutex> lock(mu_);
        if (peers_.find(from_pid) != peers_.end()) {
            while (peers_.find(next_id_) != peers_.end()) next_id_++;
            from_pid = next_id_++;
        }
        peers_.emplace(from_pid, std::make_shared<Peer>(from_pid, con, name));
    }
    LOG4CXX_INFO(logger_, "pid:" << from_pid << " success log in.");
    response(con, "success",
             {"pid", std::to_string(from_pid), "type", "logIn"});
}

void PeerManager::logOut(Type::connection_ptr con, int64_t from_pid) {
    std::lock_guard<std::mutex> lock(mu_);
    if (peers_.find(from_pid) != peers_.end()) {
        peers_.erase(from_pid);
    } else
        LOG4CXX_INFO(logger_, from_pid << " has already log out!");
    LOG4CXX_INFO(logger_, from_pid << " log out from system.");
    return;
}

void PeerManager::searchPeer(Type::connection_ptr con, int64_t from_pid,
                             int64_t dest_pid) {
    LOG4CXX_INFO(
        logger_,
        "from_pid: " << from_pid << " want to search dest_pid: " << dest_pid);
    std::lock_guard<std::mutex> plock(mu_);
    const auto& peer = peers_.find(dest_pid);
    if (peer == peers_.end()) {
        LOG4CXX_WARN(logger_, dest_pid << " not in system.");
        response(con, "pid " + std::to_string(from_pid) + " not in system.");
        return;
    }
    response(con, "success",
             {"pid", std::to_string(dest_pid), "type", "searchPeer", "name",
              peer->second->name()});
}

void PeerManager::searchPeer(Type::connection_ptr con, int64_t from_pid,
                             const std::string& name) {
    LOG4CXX_INFO(logger_,
                 "from_pid: " << from_pid << " want to search name: " << name);
    std::lock_guard<std::mutex> plock(mu_);
    for (const auto& peer : peers_) {
        if (peer.second->name() == name) {
            response(con, "success",
                     {"pid", std::to_string(peer.first), "type", "searchPeer",
                      "name", peer.second->name()});
            return;
        }
    }
    response(con, "name " + name + " not in system.");
}

void PeerManager::sendTo(Type::connection_ptr con, int64_t from_pid,
                         int64_t dest_pid, const std::string& msg) {
    LOG4CXX_INFO(logger_, "from_pid: " << from_pid << " want to send msg: "
                                       << msg << "to dest_pid: " << dest_pid);

    std::unordered_map<int64_t, std::shared_ptr<Peer>>::iterator peer;
    {
        std::lock_guard<std::mutex> plock(mu_);
        peer = peers_.find(dest_pid);
        if (peer == peers_.end()) {
            LOG4CXX_WARN(logger_, dest_pid << " not in system.");
            response(con,
                     "pid " + std::to_string(from_pid) + " not in system.");
            return;
        }
    }
    try {
        peer->second->sendMsg(msg);
    } catch (std::exception const& e) {
        LOG4CXX_ERROR(logger_, e.what());
        response(con, "failed to send msg to pid " + std::to_string(from_pid));
        peers_.erase(peer);
    }
}

std::shared_ptr<Peer> PeerManager::getPeer(int64_t pid) {
    std::lock_guard<std::mutex> plock(mu_);
    auto peer = peers_.find(pid);
    if (peer == peers_.end())
        return std::shared_ptr<Peer>();
    return peer->second;
}
