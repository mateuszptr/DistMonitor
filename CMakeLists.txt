cmake_minimum_required(VERSION 3.7)
project(DistMonitor2)

set(CMAKE_CXX_STANDARD 14)

## load in pkg-config support
find_package(PkgConfig)
## use pkg-config to get hints for 0mq locations
pkg_check_modules(PC_ZeroMQ QUIET zmq)

find_library(ZeroMQ_LIBRARY
        NAMES zmq
        PATHS ${PC_ZeroMQ_LIBRARY_DIRS}
        )

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_library(DistMonitor STATIC MonitorServer.cpp MonitorServer.h DistMonitor.cpp DistMonitor.h common.h json.hpp)

add_executable(producer ProducerConsumer/producer.cpp ProducerConsumer/pc.h  json.hpp)
target_link_libraries(producer PUBLIC ${ZeroMQ_LIBRARY} Threads::Threads DistMonitor)
add_executable(consumer ProducerConsumer/consumer.cpp ProducerConsumer/pc.h json.hpp)
target_link_libraries(consumer PUBLIC ${ZeroMQ_LIBRARY} Threads::Threads DistMonitor)