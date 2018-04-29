//
// Created by mon on 25.04.18.
//

#ifndef DISTMONITOR2_COMMON_H
#define DISTMONITOR2_COMMON_H

#include <cstdint>
#include <vector>
#include <deque>
#include <map>
#include <string>
#include "json.hpp"
#include <zmq.hpp>

typedef std::string Cond;
typedef std::uint32_t ServerId;
typedef std::uint32_t MonitorId;
typedef std::int64_t RequestNo;

struct Token {
    std::vector<RequestNo> ln;
    std::deque<ServerId> queue;
    std::map<Cond, std::deque<ServerId>> wait_queue;
};

void sendJSON(const nlohmann::json &j, zmq::socket_t &socket);

nlohmann::json recvJSON(zmq::socket_t &socket);

#endif //DISTMONITOR2_COMMON_H
