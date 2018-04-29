//
// Created by mon on 24.04.18.
//

#include "DistMonitor.h"
#include <set>
#include <iostream>

DistMonitor::DistMonitor(std::string server_name, ServerId host_id, MonitorId id, size_t n, zmq::context_t &ctx,
                         bool initializer)
        : rn(n, 1),
          monitor_id(id),
          host_id(host_id),
          validToken(initializer) {
    printf("Initialized monitor %d\n", host_id);

    mtx = new std::mutex;
    cv = new std::condition_variable;

    socket = new zmq::socket_t(ctx, ZMQ_PUSH);
    socket->connect("inproc://" + server_name);


    if (initializer) {
        token.ln = std::vector<RequestNo>(n, 1);
        token.queue = std::deque<ServerId>();
        token.wait_queue = std::map<Cond, std::deque<ServerId>>();
    }
}

void DistMonitor::recv_token(const nlohmann::json &j) {
    std::unique_lock<std::mutex> ulock(*mtx);

    assert(!processingCS);
    assert(!validToken);

    token = {j["token"]["ln"], j["token"]["queue"], j["token"]["wait_queue"]};
    data = j["data"];
    validToken = true;

    cv->notify_one();

    //std::cout << "Got token: " << j["token"].dump() << std::endl;
    //std::cout << "With data: " << j["data"].dump() << std::endl;
}


void DistMonitor::request_token(const nlohmann::json &j) {
    std::unique_lock<std::mutex> ulock(*mtx);

    ServerId rid = j["host_id"];
    //std::cout << "Got Request from" << rid << std::endl;
    rn[rid] = std::max(rn[rid], j["sn"].get<RequestNo>());
    if (validToken && !processingCS && !wantCS && rn[rid] == token.ln[rid] + 1) {
        pass_token(rid);
    }


}


void DistMonitor::pass_token(ServerId rid) {

    if (rid == host_id) return;
    assert(validToken);
    assert(!processingCS);

    if (waiting)
        token.wait_queue[waitingCond].push_back(host_id);

    nlohmann::json j;
    j["type"] = "SEND_TOKEN";
    j["remote_host_id"] = rid;
    j["data"] = data;
    j["token"] = {{"ln",         token.ln},
                  {"queue",      token.queue},
                  {"wait_queue", token.wait_queue}};
    j["monitor_id"] = monitor_id;

    validToken = false;

    sendJSON(j, *socket);

    //std::cout << "Sent token " << j["token"].dump() << " to " << rid << std::endl;
}

void DistMonitor::send_request() {
    nlohmann::json j;
    j["type"] = "SEND_REQUEST";
    j["monitor_id"] = monitor_id;
    j["sn"] = ++rn[host_id];

    sendJSON(j, *socket);
    //std::cout << "Broadcasted Request" << std::endl;
}

void DistMonitor::acquire() {
    {
        std::unique_lock<std::mutex> ulock(*mtx);

        wantCS = true;

        while (!validToken) {
            send_request();
            cv->wait(ulock);
        }

        processingCS = true;

        assert(validToken);
    }
}

void DistMonitor::release() {

    assert(validToken);
    assert(processingCS);

    std::unique_lock<std::mutex> ulock(*mtx);

    wantCS = false;
    processingCS = false;
    waiting = false;

    token.ln[host_id] = rn[host_id];

    std::set<ServerId> q(token.queue.begin(), token.queue.end());
    std::vector<ServerId> all;
    for (unsigned int i = 0; i < rn.size(); i++) all.push_back(i);
    std::vector<ServerId> dif;

    std::set_difference(all.begin(), all.end(), q.begin(), q.end(), std::inserter(dif, dif.begin()));
    for (const auto &host : dif) {
        if (rn[host] == token.ln[host] + 1) token.queue.push_back(host);
    }

    invoke_notifyAll();
    invoke_notify();

    nlohmann::json j = {{"ln",         token.ln},
                        {"queue",      token.queue},
                        {"wait_queue", token.wait_queue}};
    //std::cout << j.dump() << std::endl;

    if (!token.queue.empty()) {
        ServerId rid = token.queue.front();
        token.queue.pop_front();
        pass_token(rid);
    }

}

void DistMonitor::notify(Cond cond) {
    assert(validToken);
    assert(processingCS);
    {
        std::unique_lock<std::mutex> ulock(*mtx);
        notify_stack.push(cond);
    }
}

void DistMonitor::notifyAll(Cond cond) {
    assert(validToken);
    assert(processingCS);
    {
        std::unique_lock<std::mutex> ulock(*mtx);
        notifyAll_stack.push(cond);
    }
}

void DistMonitor::invoke_notify() {
    assert(validToken);
    //assert(processingCS);
    while (!notify_stack.empty()) {
        Cond cond = notify_stack.top();
        notify_stack.pop();
        if (!token.wait_queue[cond].empty()) {
            ServerId id = token.wait_queue[cond].front();
            token.wait_queue[cond].pop_front();

            auto iter = std::find(token.queue.begin(), token.queue.end(), id);
            assert(iter == token.queue.end());
            //token.queue.erase(iter);
            token.queue.push_front(id);
        }
    }
}

void DistMonitor::invoke_notifyAll() {
    assert(validToken);
    //assert(processingCS);
    while (!notifyAll_stack.empty()) {
        Cond cond = notifyAll_stack.top();
        notifyAll_stack.pop();
        while (!token.wait_queue[cond].empty()) {
            ServerId id = token.wait_queue[cond].front();
            token.wait_queue[cond].pop_front();

            auto iter = std::find(token.queue.begin(), token.queue.end(), id);
            assert(iter == token.queue.end());
            //token.queue.erase(iter);
            token.queue.push_front(id);
        }
    }
}

void DistMonitor::wait(Cond cond) {
    assert(validToken);
    assert(processingCS);
    std::unique_lock<std::mutex> ulock(*mtx);
    processingCS = false;
    wantCS = false;
    waiting = true;
    waitingCond = cond;

    assert(token.wait_queue[cond].empty() ||
           std::find(token.wait_queue[cond].begin(), token.wait_queue[cond].end(), host_id) ==
           token.wait_queue[cond].end());

    token.ln[host_id] = -1;


    std::set<ServerId> q(token.queue.begin(), token.queue.end());
    std::set<ServerId> all;
    for (unsigned int i = 0; i < rn.size(); i++) all.insert(i);
    std::vector<ServerId> dif;

    std::set_difference(all.begin(), all.end(), q.begin(), q.end(), std::inserter(dif, dif.begin()));
    for (const auto &host : dif) {
        if (rn[host] == token.ln[host] + 1) token.queue.push_back(host);
    }

    invoke_notifyAll();
    invoke_notify();

    nlohmann::json j = {{"ln",         token.ln},
                        {"queue",      token.queue},
                        {"wait_queue", token.wait_queue}};
    //std::cout << j.dump() << std::endl;

    if (!token.queue.empty()) {
        ServerId rid = token.queue.front();
        token.queue.pop_front();
        pass_token(rid);
    }

    //std::cout << "Waiting: " << j.dump() << std::endl;
    do {
        cv->wait(ulock);
    } while (!validToken);


    waiting = false;
    wantCS = true;
    processingCS = true;

    assert(validToken);

}

const nlohmann::json &DistMonitor::getData() const {
    assert(processingCS);
    assert(validToken);
    return data;
}

void DistMonitor::setData(const nlohmann::json &data) {
    assert(processingCS);
    assert(validToken);
    DistMonitor::data = data;
}


