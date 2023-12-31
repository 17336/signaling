cmake_minimum_required(VERSION 3.8)
project(signaling)
set(CMAKE_CXX_STANDARD 14)

file (GLOB_RECURSE SOURCE_FILES src/*.cpp)
file (GLOB_RECURSE HEADER_FILES src/*.hpp)

message("SOURCE_FILES=${SOURCE_FILES}")
message("HEADER_FILES=${HEADER_FILES}")

add_executable(signaling ${SOURCE_FILES} ${HEADER_FILES})

find_package(websocketpp REQUIRED)
if(WEBSOCKETPP_FOUND)
    target_include_directories(signaling PUBLIC ${WEBSOCKETPP_INCLUDE_DIR})
else()
    message(ERROR "websocketpp not found.")
endif()

# link boost
find_package(Boost REQUIRED)
if (Boost_FOUND)
    target_include_directories(signaling PUBLIC ${Boost_INCLUDE_DIR})
    link_directories(signaling ${Boost_LIBRARY_DIRS})
    find_library(LIBS boost_system pthread)
    message("LIBS=${LIBS}")
    target_link_libraries(signaling PRIVATE ${LIBS})
else()
    message(ERROR "boost not found.")
endif()

# link log4cxx
find_package(log4cxx REQUIRED)
if(log4cxx_FOUND)
	message("found log4cxx, include dir is ${loc4cxx_INCLUDE_DIR}")
    target_include_directories(signaling PUBLIC $<TARGET_PROPERTY:log4cxx,INTERFACE_INCLUDE_DIRECTORIES>)
    target_link_libraries(signaling PRIVATE log4cxx)
else()
    message(ERROR "loc4cxx not found")
endif()

find_library(MYSQLCPPCONN mysqlcppconn)

if(MYSQLCPPCONN)
    message(STATUS "Found mysqlcppconn: ${MYSQLCPPCONN}")
    target_link_libraries(signaling PUBLIC ${MYSQLCPPCONN})
else()
    message(FATAL_ERROR "mysqlcppconn not found")
endif()

find_package(RapidJSON REQUIRED)
target_include_directories(signaling PUBLIC ${RapidJSON_INCLUDE_DIRS})