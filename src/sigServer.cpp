#include "sigServer.h"

#include "log4cxx/logger.h"

#include <sstream>
#include <string>

#include "workerPool.h"

using websocketpp::lib::bind;
using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;


log4cxx::LoggerPtr sigServer::logger_ = log4cxx::Logger::getRootLogger();

sigServer::sigServer() : workers_() {
    // Initialize Asio Transport
    m_server_.init_asio();

    // Register handler callbacks
    m_server_.set_message_handler(
        bind(&sigServer::on_message, this, ::_1, ::_2));
}

void sigServer::run(uint16_t port) {
    // listen on specified port
    m_server_.listen(port);

    // Start the server accept loop
    m_server_.start_accept();

    // Start the ASIO io_service run loop
    try {
        workers_.start();
        m_server_.run();
    } catch (const std::exception &e) {
        std::cout << e.what() << std::endl;
    }
}

void sigServer::on_open(Type::connection_hdl hdl) {}

void sigServer::on_close(Type::connection_hdl hdl) {}

void sigServer::on_message(Type::connection_hdl hdl, Type::message_ptr msg) {
    Type::connection_ptr con = m_server_.get_con_from_hdl(hdl);
    workers_.addContext(Context(con, msg));
    LOG4CXX_INFO(logger_, "get context");
}