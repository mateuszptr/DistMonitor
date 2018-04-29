//
// Created by mon on 25.04.18.
//

#ifndef DISTMONITOR2_PC_H
#define DISTMONITOR2_PC_H

#include "../DistMonitor.h"
#include "../MonitorServer.h"
#include "pc.h"
#include <cstdio>
#include <queue>
#include <deque>
#include <chrono>
#include <random>

void random_sleep() {
    using namespace std::chrono;

    std::default_random_engine engine;
    std::uniform_int_distribution<int> distribution(1,1000);

    std::this_thread::sleep_for(milliseconds(distribution(engine)));
}

void init_monitor(DistMonitor &monitor) {
    monitor.acquire();

    nlohmann::json j = R"({"buffer": []})"_json;

    monitor.setData(j);

    monitor.release();
}

#endif //DISTMONITOR2_PC_H
