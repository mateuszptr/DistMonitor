//
// Created by mon on 24.04.18.
//

#ifndef DISTMONITOR2_MONITORSERVER_H
#define DISTMONITOR2_MONITORSERVER_H


#include <vector>
#include <string>
#include <cstdint>
#include <thread>
#include <zmq.hpp>
#include "DistMonitor.h"
#include "common.h"

class MonitorServer {
private:
    // ID
    ServerId host_id;
    // Config
    std::string name;
    std::vector<std::string> hosts;
    std::vector<std::uint16_t> ports_sub;
    std::vector<std::uint16_t> ports_pull;
//    std::uint16_t port_sub, port_pull;
    // ZMQ Sockets
    zmq::context_t ctx = zmq::context_t(1);
    zmq::socket_t pull = zmq::socket_t(ctx, ZMQ_PULL);
    zmq::socket_t pub = zmq::socket_t(ctx, ZMQ_PUB);
    zmq::socket_t sub = zmq::socket_t(ctx, ZMQ_SUB);
    zmq::socket_t inproc = zmq::socket_t(ctx, ZMQ_PULL);
    std::vector<zmq::socket_t> pushes;
    // Monitors
    std::map<MonitorId, DistMonitor> monitors;
    // Thread
    std::thread *thread;
private: // Functions
    void load_config(const std::string &conf);

    void connect_sockets();

    void hello_master();

    void hello_slave();

    void poll();

    void handle_pull(const nlohmann::json &j);

    void handle_sub(const nlohmann::json &j);

    void handle_inproc(const nlohmann::json &j);

    nlohmann::json basic_message(std::string type);

    void thread_routine(std::string conf, ServerId id);

public:
    MonitorServer(std::string conf, ServerId id);

    ~MonitorServer() = default;

    DistMonitor &register_monitor(MonitorId id, bool is_init);
};


#endif //DISTMONITOR2_MONITORSERVER_H
