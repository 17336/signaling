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
        log4cxx::PropertyConfigurator::configure("../conf/log.conf");
        sigServer server;
        server.run(9000);
    }
    catch (std::exception const &e)
    {
        std::cout << e.what() << std::endl;
    }
}
