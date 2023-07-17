#include "sigServer.h"

#include <iostream>
#include "log4cxx/level.h"
#include <log4cxx/logger.h>
#include <log4cxx/basicconfigurator.h>
#include <log4cxx/propertyconfigurator.h>
#include <log4cxx/helpers/exception.h>
#include <exception>
#include <thread>
#include <functional>

int main()
{
    try
    {
        // Create a server endpoint
        //%-4r [%t] %%-5p %c %x - %m%n
        log4cxx::BasicConfigurator::configure();
        log4cxx::LoggerPtr logger = log4cxx::Logger::getRootLogger();
        sigServer server;

        std::thread t1(std::bind(&sigServer::processTasks, &server));
        std::thread t2(std::bind(&sigServer::processTasks, &server));
        server.run(9000);
        t1.join();
        t2.join();
    }
    catch (std::exception const &e)
    {
        std::cout << e.what() << std::endl;
    }
}
