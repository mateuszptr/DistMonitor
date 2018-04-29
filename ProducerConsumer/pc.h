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

void init_monitor(DistMonitor &monitor) {
    monitor.acquire();

    nlohmann::json j = R"({"buffer": []})"_json;

    monitor.setData(j);

    monitor.release();
}

#endif //DISTMONITOR2_PC_H
