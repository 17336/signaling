cmake_minimum_required(VERSION 3.8)
project(signaling)

set(WEBSOCKETPP_INCLUDE /usr/local/include/websocketpp)

file (GLOB_RECURSE SOURCE_FILES src/*.cpp)
file (GLOB_RECURSE HEADER_FILES src/*.hpp)

message("SOURCE_FILES=${SOURCE_FILES}")
message("HEADER_FILES=${HEADER_FILES}")

add_executable(signaling ${SOURCE_FILES} ${HEADER_FILES})

# link boost
find_package(Boost REQUIRED)
IF (Boost_FOUND)
    target_include_directories(signaling PUBLIC ${Boost_INCLUDE_DIR})
    link_directories(signaling ${Boost_LIBRARY_DIRS})
    find_library(LIBS boost_system pthread)
    message("LIBS=${LIBS}")
    target_link_libraries(signaling PRIVATE ${LIBS})
else()
    message(WARNING "boost not found.")
ENDIF()

# link log4cxx
find_package(log4cxx REQUIRED)
if(log4cxx_FOUND)
	message("found log4cxx, include dir is ${loc4cxx_INCLUDE_DIR}")
    target_include_directories(signaling PUBLIC $<TARGET_PROPERTY:log4cxx,INTERFACE_INCLUDE_DIRECTORIES>)
    target_link_libraries( signaling PRIVATE log4cxx)
else()
    message(WARNING "loc4cxx not found")
endif()

find_package(RapidJSON REQUIRED)
target_include_directories(signaling PUBLIC ${RapidJSON_INCLUDE_DIRS})