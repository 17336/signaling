#include "sessionDumper.h"

log4cxx::LoggerPtr SessionDumper::logger_ =
    log4cxx::Logger::getLogger("server");

SessionDumper* SessionDumper::getInstance() {
    static SessionDumper dumper;
    return &dumper;
}

SessionDumper::SessionDumper() : start_(true), t_(&SessionDumper::run, this) {
    LOG4CXX_INFO(logger_, "start session dumper.");
}

SessionDumper::~SessionDumper() {
    start_ = false;
    t_.join();
    LOG4CXX_INFO(logger_, "stop session dumper.");
}

void SessionDumper::addSessionLog(const SessionLog &l) {
    input_.push(l);
}

void SessionDumper::run() {
    SessionLog log;
    while (start_) {
        if (!input_.get(&log, 5000)) {
            LOG4CXX_INFO(logger_, "No SessionLog in input");
            continue;
        }
        try {
            dump(log);
        } catch (const std::exception &e) {
            LOG4CXX_ERROR(logger_, "failed to dump session log to sql because:"
                                       << e.what());
        }
    }
}

void SessionDumper::dump(SessionLog &l) {
    LOG4CXX_INFO(logger_, "dump session that room id is " << l.room_id_);
    auto pool = ConnPool::getInstance();
    auto con = pool->getConnection();
    sql::PreparedStatement *prep_stmt;
    prep_stmt = con->prepareStatement(
        "INSERT INTO session (room_id,start_time,end_time) VALUES (?,?,?)");
    prep_stmt->setInt64(1, l.room_id_);
    prep_stmt->setDateTime(2, l.start_time_);
    prep_stmt->setDateTime(3, l.end_time_);
    int rows_affected = prep_stmt->executeUpdate();
    if (rows_affected <= 0) {
        LOG4CXX_WARN(logger_,
                     "Insert session status failed. No rows affected.");
        return;
    }
    delete prep_stmt;

    auto stmt = con->createStatement();
    // 获取最后插入的自增主键值
    sql::ResultSet *result =
        stmt->executeQuery("SELECT LAST_INSERT_ID() AS last_id");
    int last_id = -1;
    if (result->next()) {
        last_id = result->getInt("last_id");
        LOG4CXX_INFO(logger_, "last inserted ID: " << last_id);
    }

    prep_stmt = con->prepareStatement(
        "INSERT INTO session_member "
        "(session_id,peer_id,name,ip,join_time,left_time,open_camera,open_"
        "audio,open_screen,connected) VALUES (?,?,?,?,?,?,?,?,?,?)");
    int len = l.peers.size();
    for (int i = 0; i < len; i++) {
        prep_stmt->setInt(1, last_id);
        prep_stmt->setInt64(2, l.peers[i].id_);
        prep_stmt->setString(3, l.peers[i].name_);
        prep_stmt->setString(4, l.peers[i].ip_);
        LOG4CXX_DEBUG(logger_, "join time"<<l.statuses[i].join_time_);
        prep_stmt->setDateTime(5, l.statuses[i].join_time_);
        LOG4CXX_DEBUG(logger_, "left time"<<l.statuses[i].left_time_);
        prep_stmt->setDateTime(6, l.statuses[i].left_time_);
        prep_stmt->setBoolean(7, l.statuses[i].isCameraUsed());
        prep_stmt->setBoolean(8, l.statuses[i].isAudioUsed());
        prep_stmt->setBoolean(9, l.statuses[i].isScreenUsed());
        prep_stmt->setBoolean(10, l.statuses[i].isConnected());
        int rows_affected = prep_stmt->executeUpdate();
        if (rows_affected <= 0) {
            LOG4CXX_WARN(logger_,
                         "Insert peer status failed. No rows affected.");
        }
    }
    delete prep_stmt;
}
