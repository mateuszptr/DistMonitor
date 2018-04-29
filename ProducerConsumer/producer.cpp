//
// Created by mon on 25.04.18.
//

#include "../DistMonitor.h"
#include "../MonitorServer.h"
#include "pc.h"
#include <iostream>

const size_t MAX_N = 100;
const char *FULL_COND = "FULL";
const char *EMPTY_COND = "EMPTY";
const MonitorId MID = 1337;

std::string produce(int id, int sn) {
    std::string prod = std::to_string(id) + "_" + std::to_string(sn);
    //std::cout << "Produkuję: " << prod << std::endl;

    return prod;
}

void emplace(DistMonitor &monitor, const std::string &prod) {
    monitor.acquire();
    {
        nlohmann::json data;
        std::deque<std::string> q;
        size_t size = MAX_N + 1;
        while (size >= MAX_N) {
            data = monitor.getData();
            q = data["buffer"].get<decltype(q)>();
            size = q.size();

            if (q.size() == MAX_N)
                monitor.wait(FULL_COND);
        }

        q.push_back(prod);
        monitor.notifyAll(EMPTY_COND);

        data.clear();
        data["buffer"] = nlohmann::json(q);

        monitor.setData(data);
        std::cout << data.dump() << std::endl;

    }
    monitor.release();
}


int main(int argc, char **argv) {

    ServerId id = static_cast<ServerId>(std::stoul(argv[1]));
    bool init = (std::string(argv[2]) == "true");
    MonitorServer server("pc.json", id);
    DistMonitor &monitor = server.register_monitor(MID, init);

    if (init) init_monitor(monitor);

    int sn = 0;
    while (1) {
        std::string prod = produce(id, sn++);
        emplace(monitor, prod);
        //zmq_sleep(1);
    }
}