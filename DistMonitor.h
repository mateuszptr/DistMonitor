//
// Created by mon on 24.04.18.
//

#ifndef DISTMONITOR2_DISTMONITOR_H
#define DISTMONITOR2_DISTMONITOR_H

#include <vector>
#include <cstdint>
#include <deque>
#include <string>
#include <map>
#include <zmq.hpp>
#include <mutex>
#include <condition_variable>
#include "json.hpp"
#include <stack>
#include "common.h"


class DistMonitor {
    friend class MonitorServer;

private: // Fields
    // Token
    Token token;
    bool validToken = false;
    bool processingCS = false;
    bool wantCS = false;
    bool waiting = false;
    Cond waitingCond;
    // RN
    std::vector<RequestNo> rn;
    // Communication with server
    zmq::socket_t *socket{};
    MonitorId monitor_id{};
    ServerId host_id{};
    // Locks
    std::mutex *mtx{};
    std::condition_variable *cv{};
    // Data
    nlohmann::json data;
public:
    void setData(const nlohmann::json &data);

    const nlohmann::json &getData() const;

private:
    // Invoke notify
    std::stack<Cond> notify_stack;
    std::stack<Cond> notifyAll_stack;
private: // Functions


    void recv_token(const nlohmann::json &j);

    void request_token(const nlohmann::json &j);

    void pass_token(ServerId rid);

    void send_request();

    void invoke_notify();

    void invoke_notifyAll();

public: // Functions
    DistMonitor(std::string server_name, ServerId host_id, MonitorId id, size_t n, zmq::context_t &ctx,
                bool initializer);

    DistMonitor() = default;

    void acquire();

    void release();

    void notify(Cond cond);

    void notifyAll(Cond cond);

    void wait(Cond cond);


};


#endif //DISTMONITOR2_DISTMONITOR_H
