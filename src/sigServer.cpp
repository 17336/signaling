#include "sigServer.h"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include <log4cxx/logger.h>
#include <sstream>
#include <string>

using websocketpp::lib::bind;
using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;

using websocketpp::lib::lock_guard;
using websocketpp::lib::thread;
using websocketpp::lib::unique_lock;

sigServer::sigServer() : next_peer_id_(0)
{
    logger = log4cxx::Logger::getRootLogger();
    // Initialize Asio Transport
    m_server_.init_asio();

    // Register handler callbacks
    m_server_.set_message_handler(bind(&sigServer::on_message, this, ::_1, ::_2));
}

void sigServer::run(uint16_t port)
{
    // listen on specified port
    m_server_.listen(port);

    // Start the server accept loop
    m_server_.start_accept();

    // Start the ASIO io_service run loop
    try
    {
        m_server_.run();
    }
    catch (const std::exception &e)
    {
        std::cout << e.what() << std::endl;
    }
}

bool sigServer::checkLog(connection_hdl hdl, int64_t pid)
{
    lock_guard<mutex> plock(peer_lock_);
    if (peers_.find(pid) == peers_.end())
    {
        LOG4CXX_WARN(logger, pid << " not found in peers.");
        m_server_.send(hdl, "pid not found", opcode::TEXT);
        return false;
    }
    return true;
}

bool sigServer::checkCreate(connection_hdl hdl, int64_t rid)
{
    lock_guard<mutex> rlock(room_lock_);
    if (rooms_.find(rid) == rooms_.end())
    {
        LOG4CXX_WARN(logger, rid << " not found in rooms.");
        m_server_.send(hdl, "rid not found", opcode::TEXT);
        return false;
    }
    return true;
}

void sigServer::logIn(connection_hdl hdl, int64_t pid, const std::string &name)
{
    {
        lock_guard<mutex> lock(peer_lock_);
        LOG4CXX_INFO(logger, "next "<< next_peer_id_);
        if (peers_.find(pid) != peers_.end())
        {
            while (peers_.find(next_peer_id_) != peers_.end())
                next_peer_id_++;
            pid = next_peer_id_++;
        }
        peers_.emplace(pid, peer(&(this->m_server_), pid, hdl, name));
    }
    LOG4CXX_INFO(logger, "pid:" << pid << " log in.");
    std::stringstream ss;
    ss << "{\"type\":\"logIn\",\"pid\":" << pid << "}";
    opcode code = opcode::TEXT;
    m_server_.send(hdl, ss.str(), code);
}

void sigServer::logOut(connection_hdl hdl, int64_t pid)
{
    lock_guard<mutex> lock(peer_lock_);
    if (peers_.find(pid) != peers_.end())
    {
        peers_.erase(pid);
    }
    else 
        LOG4CXX_INFO(logger, pid << " has already log out!");
    LOG4CXX_INFO(logger, pid << " log out from signaling.");
    return;
}

void sigServer::createRoom(connection_hdl hdl, int64_t pid)
{
    lock_guard<mutex> plock(peer_lock_);
    if (peers_.find(pid) == peers_.end())
    {
        LOG4CXX_WARN(logger, pid << " not found in peers.");
        m_server_.send(hdl, "pid not found", opcode::TEXT);
        return;
    }
    int64_t rid;
    {
        lock_guard<mutex> rlock(room_lock_);
        while (rooms_.find(next_room_id_) != rooms_.end())
            next_room_id_++;
        rid = next_room_id_++;
        room r(rid);
        r.addPeer(pid);
        rooms_[rid] = r;
    }
    std::stringstream ss;
    ss << "{\"type\":\"createRoom\",\"rid\":" << rid << "}";
    LOG4CXX_WARN(logger, "room " << rid << " created by " << pid);
    m_server_.send(hdl, ss.str(), opcode::TEXT);
}

void sigServer::joinRoom(connection_hdl hdl, int64_t pid, int64_t rid)
{
    if (!checkLog(hdl, pid) || !checkCreate(hdl, rid))
        return;
    // peer加入room
    {
        lock_guard<mutex> rlock(room_lock_);
        rooms_[rid].addPeer(pid);
    }

    // 广播
    std::stringstream ss;
    ss << "{\"type\":\"joined\",\"pid\":" << pid << ",\"rid\":"<<rid<<"}";
    sendToRoom(hdl, rid, ss.str());
}

void sigServer::leftRoom(connection_hdl hdl, int64_t pid, int64_t rid)
{
    if (!checkLog(hdl, pid) || !checkCreate(hdl, rid))
        return;
    {
        lock_guard<mutex> rlock(room_lock_);
        rooms_[rid].removePeer(pid);
    }

    std::stringstream ss;
    ss << "{\"type\":\"left\",\"pid\":" << pid << ",\"rid\":"<<rid<<"}";
    sendToRoom(hdl, rid, ss.str());
}

void sigServer::sendTo(connection_hdl hdl, int64_t pid, std::string msg)
{
    if (!checkLog(hdl, pid))
        return;
    lock_guard<mutex> plock(peer_lock_);
    peer &p = peers_[pid];
    try
    {
        p.sendMsg(msg);
    }
    catch (std::exception const &e)
    {
        if (e.what() == std::string("Bad Connection"))
        {
            LOG4CXX_WARN(logger, pid << "has closed!");
            peers_.erase(pid);
            m_server_.send(hdl, "pid not online", opcode::TEXT);
        }
        else
            throw e;
    }
}

void sigServer::sendToRoom(connection_hdl hdl, int64_t from_pid, int64_t rid, std::string msg)
{
    if (!checkCreate(hdl, rid))
        return;
    lock_guard<mutex> rlock(room_lock_);
    room &r = rooms_[rid];
    lock_guard<mutex> pids_lock(r.pids_mutex_);
    for (auto it = r.pids_.begin(); it != r.pids_.end();)
    {
        int64_t pid = *it;
        if (pid == from_pid) {
            it++;
            continue;
        }
        lock_guard<mutex> plock(peer_lock_);
        if (peers_.find(pid) == peers_.end())
        {
            LOG4CXX_WARN(logger, pid << " not found in room " << rid);
            it = r.pids_.erase(it);
        }
        else
        {
            try
            {
                peers_[pid].sendMsg(msg);
            }
            catch (std::exception const &e)
            {
                if (e.what() == std::string("Bad Connection"))
                {
                    LOG4CXX_WARN(logger, pid << "has closed!");
                    peers_.erase(*it);
                    it = r.pids_.erase(it);
                    continue;
                }
                else
                    throw e;
            }
            it++;
        }
    }
}

void sigServer::sendToRoom(connection_hdl hdl, int64_t rid, std::string msg)
{
    if (!checkCreate(hdl, rid))
        return;
    lock_guard<mutex> rlock(room_lock_);
    room &r = rooms_[rid];
    lock_guard<mutex> pids_lock(r.pids_mutex_);
    for (auto it = r.pids_.begin(); it != r.pids_.end();)
    {
        int64_t pid = *it;
        lock_guard<mutex> plock(peer_lock_);
        if (peers_.find(pid) == peers_.end())
        {
            LOG4CXX_WARN(logger, pid << " not found in room " << rid);
            it = r.pids_.erase(it);
        }
        else
        {
            try
            {
                peers_[pid].sendMsg(msg);
            }
            catch (std::exception const &e)
            {
                if (e.what() == std::string("Bad Connection"))
                {
                    LOG4CXX_WARN(logger, pid << "has closed!");
                    peers_.erase(*it);
                    it = r.pids_.erase(it);
                    continue;
                }
                else
                    throw e;
            }
            it++;
        }
    }
}

void sigServer::getPeersInRoom(connection_hdl hdl, int64_t rid)
{
    lock_guard<mutex> rlock(room_lock_);
    if (rooms_.find(rid) == rooms_.end())
    {
        LOG4CXX_WARN(logger, rid << " not found in rooms.");
        m_server_.send(hdl, "rid not found", opcode::TEXT);
        return;
    }
    room &r = rooms_[rid];
    rapidjson::Document d;
    d.SetObject();
    rapidjson::Value pids(rapidjson::kArrayType);
    lock_guard<mutex> pids_lock(r.pids_mutex_);
    for (auto pid : r.pids_)
    {
        pids.PushBack(pid, d.GetAllocator());
    }
    d.AddMember("type", "getPeersInRoom", d.GetAllocator());
    d.AddMember("pids", pids, d.GetAllocator());
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    d.Accept(writer);
    LOG4CXX_DEBUG(logger, "room's peers: " << rooms_[rid].pids_.size() << buffer.GetString());
    m_server_.send(hdl, buffer.GetString(), opcode::TEXT);
}

//
void sigServer::processTasks()
{
    while (true)
    {
        unique_lock<mutex> lock(tasks_lock_);
        tasks_cond_.wait(lock, [=]
                         { return !(this->tasks_.empty()); });
        task t = tasks_.front();
        tasks_.pop();
        lock.unlock();
        message_ptr msg = t.msg;
        connection_hdl &hdl = t.hdl;

        rapidjson::Document doc;
        // payload:{"operate":xxx,"body":xxx, ...}
        std::string payload = msg->get_payload();
        doc.Parse(payload.c_str());
        if (doc.HasParseError() || !doc.IsObject())
        {
            m_server_.send(hdl, "need json", opcode::text);
            continue;
        }
        if (doc.HasMember("operate") && doc["operate"].IsInt())
        {
            int opt = doc["operate"].GetInt();
            switch (opt)
            {
            case operate::LOG_IN:
            {

                int64_t pid = 0;
                std::string name;
                if (doc.HasMember("pid") && doc["pid"].IsInt())
                {
                    pid = doc["pid"].GetInt64();
                }
                if (doc.HasMember("name") && doc["name"].IsString())
                {
                    name = doc["name"].GetString();
                }
                logIn(hdl, pid, name);
                break;
            }
            case operate::LOG_OUT:
                if (doc.HasMember("pid") && doc["pid"].IsInt())
                {
                    int64_t pid = doc["pid"].GetInt64();
                    logOut(hdl, pid);
                }
                else
                {
                    m_server_.send(hdl, "miss argument: pid", opcode::TEXT);
                }
                break;
            case operate::CREATE_ROOM:
                if (doc.HasMember("pid") && doc["pid"].IsInt())
                {
                    int64_t pid = doc["pid"].GetInt64();
                    createRoom(hdl, pid);
                }
                else
                {
                    m_server_.send(hdl, "miss argument: pid", opcode::TEXT);
                }
                break;
            case operate::JOIN_ROOM:
                if (doc.HasMember("pid") && doc["pid"].IsInt() &&
                    doc.HasMember("rid") && doc["rid"].IsInt())
                {
                    int64_t pid = doc["pid"].GetInt64();
                    int64_t rid = doc["rid"].GetInt64();
                    joinRoom(hdl, pid, rid);
                }
                else
                {
                    m_server_.send(hdl, "miss argument: pid or rid", opcode::TEXT);
                }
                break;
            case operate::LEFT_ROOM:
                if (doc.HasMember("pid") && doc["pid"].IsInt() &&
                    doc.HasMember("rid") && doc["rid"].IsInt())
                {
                    int64_t pid = doc["pid"].GetInt64();
                    int64_t rid = doc["rid"].GetInt64();
                    leftRoom(hdl, pid, rid);
                }
                else
                {
                    m_server_.send(hdl, "miss argument: pid or rid", opcode::TEXT);
                }
                break;
            case operate::SEND_TO:
                if (doc.HasMember("pid") && doc["pid"].IsInt() &&
                    doc.HasMember("body") && doc["body"].IsString())
                {
                    int64_t pid = doc["pid"].GetInt64();
                    std::string body = doc["body"].GetString();
                    sendTo(hdl, pid, body);
                }
                else
                {
                    m_server_.send(hdl, "miss argument: pid or body", opcode::TEXT);
                }
                break;
            case operate::SEND_TO_ROOM:
                if (doc.HasMember("pid") && doc["pid"].IsInt() && doc.HasMember("rid") && doc["rid"].IsInt() &&
                    doc.HasMember("body") && doc["body"].IsString())
                {
                    int64_t pid = doc["pid"].GetInt64();
                    int64_t rid = doc["rid"].GetInt64();
                    std::string body = doc["body"].GetString();
                    sendToRoom(hdl, pid, rid, body);
                }
                else
                {
                    m_server_.send(hdl, "miss argument: pid or body", opcode::TEXT);
                }
                break;
            case operate::GET_PEERS_IN_ROOM:
                if (doc.HasMember("rid") && doc["rid"].IsInt())
                {
                    int64_t rid = doc["rid"].GetInt64();
                    getPeersInRoom(hdl, rid);
                }
                else
                {
                    m_server_.send(hdl, "miss argument: rid", opcode::TEXT);
                }
                break;
            default:
                m_server_.send(hdl, "operate not support", opcode::TEXT);
            }
        }
        else
        {
            m_server_.send(hdl, "operate not support", opcode::TEXT);
        }
    }
}

void sigServer::on_open(connection_hdl hdl)
{
}

void sigServer::on_close(connection_hdl hdl)
{
}

void sigServer::on_message(connection_hdl hdl, server::message_ptr msg)
{
    task t(hdl, msg);
    {
        lock_guard<mutex> lock(tasks_lock_);
        tasks_.push(t);
    }
    tasks_cond_.notify_one();
    LOG4CXX_INFO(logger, "debug:"<<msg->get_payload());
}