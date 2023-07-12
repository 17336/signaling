cmake_minimum_required(VERSION 2.8)
project(signaling)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/CMakeHelpers.cmake)

set(WEBSOCKETPP_INCLUDE /usr/local/include/websocketpp)

file (GLOB_RECURSE SOURCE_FILES src/*.cpp)
file (GLOB_RECURSE HEADER_FILES src/*.hpp)

message("SOURCE_FILES=${SOURCE_FILES}")
message("HEADER_FILES=${HEADER_FILES}")

add_executable(signaling ${SOURCE_FILES} ${HEADER_FILES})
find_package(Boost REQUIRED)
IF (Boost_FOUND)
    target_include_directories(signaling PUBLIC ${Boost_INCLUDE_DIR})
    link_directories(signaling ${Boost_LIBRARY_DIRS})
    target_link_libraries(signaling boost_system)
ENDIF()