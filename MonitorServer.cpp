//
// Created by mon on 24.04.18.
//

#include "MonitorServer.h"
#include <fstream>

MonitorServer::MonitorServer(std::string conf, ServerId id) {
    host_id = id;
    load_config(conf);
    connect_sockets();
    thread = new std::thread(&MonitorServer::thread_routine, this, conf, id);
}

void MonitorServer::load_config(const std::string &conf) {
    std::ifstream is(conf);
    nlohmann::json j;
    is >> j;
    std::vector<std::tuple<std::string, std::uint16_t, std::uint16_t>> tuples = j["hosts"];
    for (auto &tup : tuples) {
        this->hosts.push_back(std::get<0>(tup));
        this->ports_sub.push_back(std::get<1>(tup));
        this->ports_pull.push_back(std::get<2>(tup));
    }
    //this->hosts = j["hosts"].get<decltype(hosts)>();
    this->name = j["name"];
}

void MonitorServer::thread_routine(const std::string conf, ServerId id) {

    if (id == 0) {
        hello_master();
    } else {
        hello_slave();
    }

    poll();

}

void MonitorServer::connect_sockets() {
    for (const auto &host : hosts) {
        pushes.emplace_back(ctx, ZMQ_PUSH);
    }
    pub.bind("tcp://" + hosts[host_id] + ":" + std::to_string(ports_sub[host_id]));
    pull.bind("tcp://" + hosts[host_id] + ":" + std::to_string(ports_pull[host_id]));
    inproc.bind("inproc://" + name);

    for (auto i = 0; i < pushes.size(); i++) {
        if (i != host_id) {
            pushes[i].connect("tcp://" + hosts[i] + ":" + std::to_string(ports_pull[i]));
            sub.connect("tcp://" + hosts[i] + ":" + std::to_string(ports_sub[i]));
            sub.setsockopt(ZMQ_SUBSCRIBE, "", 0);
        }
    }
}

void MonitorServer::hello_master() {
    bool ready[hosts.size()] = {false};
    ready[host_id] = true;

    while (std::any_of(ready, ready + hosts.size(), [](bool b) { return !b; })) {
        sendJSON(basic_message("HELLO"), pub);
        using namespace std::chrono;

        zmq::pollitem_t pollitem = {pull, 0, ZMQ_POLLIN, 0};
        milliseconds ms(100);
        while (ms.count() > 0) {
            auto ts1 = system_clock::now();
            zmq::poll(&pollitem, 1, ms);
            auto ts2 = system_clock::now();
            ms -= duration_cast<milliseconds>(ts2 - ts1);

            if (pollitem.revents & ZMQ_POLLIN) {
                nlohmann::json rj = recvJSON(pull);
                if (rj["type"] == "READY") {
                    ServerId host_id = rj["host_id"];
                    ready[host_id] = true;
                }
            }
        }
    }

    sendJSON(basic_message("ALL_READY"), pub);
}


nlohmann::json MonitorServer::basic_message(std::string type) {
    return {{"type",    type},
            {"host_id", host_id}};
}

void sendJSON(const nlohmann::json &j, zmq::socket_t &socket) {
    auto vec = nlohmann::json::to_cbor(j);
    socket.send(vec.data(), vec.size());
}

nlohmann::json recvJSON(zmq::socket_t &socket) {
    zmq::message_t message;
    socket.recv(&message);
    return nlohmann::json::from_cbor((char *) message.data(), message.size());
}

void MonitorServer::hello_slave() {
    std::string type;
    ServerId rid = 0;
    while (type != "HELLO") {
        nlohmann::json j = recvJSON(sub);
        rid = j["host_id"];
        if (rid == 0) type = j["type"];
    }

    sendJSON(basic_message("READY"), pushes[rid]);

    while (type != "ALL_READY") {
        nlohmann::json j = recvJSON(sub);
        rid = j["host_id"];
        if (rid == 0) type = j["type"];
    }
}

void MonitorServer::poll() {
    zmq::pollitem_t pollitems[] = {
            {pull,   0, ZMQ_POLLIN, 0},
            {sub,    0, ZMQ_POLLIN, 0},
            {inproc, 0, ZMQ_POLLIN, 0}
    };

    bool quit = false;
    while (!quit) {
        zmq_poll(pollitems, 3, -1);
        nlohmann::json j;
        if (pollitems[0].revents & ZMQ_POLLIN) {
            j = recvJSON(pull);
            handle_pull(j);

        }
        if (pollitems[1].revents & ZMQ_POLLIN) {
            j = recvJSON(sub);
            handle_sub(j);

        }
        if (pollitems[2].revents & ZMQ_POLLIN) {
            j = recvJSON(inproc);
            handle_inproc(j);
        }
    }
}

void MonitorServer::handle_pull(const nlohmann::json &j) {
    if (j["type"] == "PASS_TOKEN") {
        monitors[j["monitor_id"]].recv_token(j);
    }
}

void MonitorServer::handle_sub(const nlohmann::json &j) {
    if (j["type"] == "REQUEST") {
        monitors[j["monitor_id"]].request_token(j);
    }
}

void MonitorServer::handle_inproc(const nlohmann::json &j) {
    if (j["type"] == "SEND_TOKEN") {
        nlohmann::json nj = basic_message("PASS_TOKEN");
        nj["monitor_id"] = j["monitor_id"];
        nj["token"] = j["token"];
        nj["data"] = j["data"];

        sendJSON(nj, pushes[j["remote_host_id"]]);
    } else if (j["type"] == "SEND_REQUEST") {
        nlohmann::json nj = basic_message("REQUEST");
        nj["sn"] = j["sn"];
        nj["monitor_id"] = j["monitor_id"];

        sendJSON(nj, pub);
    }
}

DistMonitor &MonitorServer::register_monitor(MonitorId id, bool is_init) {
    //monitors.emplace(id, DistMonitor(name, host_id, id, hosts.size(), ctx, is_init));
    monitors.emplace(std::piecewise_construct,
                     std::forward_as_tuple(id),
                     std::forward_as_tuple(name, host_id, id, hosts.size(), ctx, is_init));
    //monitors.insert(std::make_pair(id, DistMonitor(name, host_id, id, hosts.size(), ctx, is_init)));
    return monitors[id];
}




