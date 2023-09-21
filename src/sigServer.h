#ifndef _SIGSERVER_H_
#define _SIGSERVER_H_

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include <iostream>
#include <unordered_map>
#include <vector>
#include "log4cxx/logger.h"
#include <queue>

#include "workerPool.h"
#include "type.h"


class sigServer
{
public:
    // pull out the type of messages sent by our config
    sigServer(/* args */);

    void on_open(Type::connection_hdl hdl);
    void on_close(Type::connection_hdl hdl);
    void on_message(Type::connection_hdl hdl, Type::message_ptr msg);

    void run(uint16_t port);

private:
    WorkerPool workers_;
    Type::server m_server_;
    static log4cxx::LoggerPtr logger_;
};

#endif // _SIGSERVER_H_