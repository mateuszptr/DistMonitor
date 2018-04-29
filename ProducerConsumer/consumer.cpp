//
// Created by mon on 25.04.18.
//

#include "pc.h"
#include <iostream>

const size_t MAX_N = 100;
const char *FULL_COND = "FULL";
const char *EMPTY_COND = "EMPTY";
const MonitorId MID = 1337;

void consume(const std::string &prod) {
    std::cout << "Konsumuję: " << prod << std::endl;
}

std::string remove(DistMonitor &monitor) {
    monitor.acquire();

    nlohmann::json data;
    std::deque<std::string> q;
    size_t size = 0;
    while (size == 0) {
        data = monitor.getData();
        q = data["buffer"].get<decltype(q)>();
        size = q.size();

        if (q.empty()) monitor.wait(EMPTY_COND);
    }

    std::string prod = q.front();
    q.pop_front();
    monitor.notifyAll(FULL_COND);

    //std::cout << data.dump() << std::endl;
    data.clear();
    data["buffer"] = q;

    monitor.setData(data);

    monitor.release();

    return prod;
}


int main(int argc, char **argv) {

    ServerId id = static_cast<ServerId>(std::stoul(argv[1]));
    bool init = (std::string(argv[2]) == "true");
    MonitorServer server("pc.json", id);
    DistMonitor &monitor = server.register_monitor(MID, init);

    if (init) init_monitor(monitor);

    int sn = 0;
    while (1) {
        std::string prod = remove(monitor);
        consume(prod);
        zmq_sleep(1);
    }
}