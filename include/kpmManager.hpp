#ifndef KPMMANAGER
#define KPMMANAGER

#include "../libs/flexric/src/xApp/e42_xapp_api.h"
#include "spdlog/spdlog.h"
#include "defer.hpp"

#include "chrono"
#include <cassert>
#include <iostream>
#include <cstdlib>

namespace KpmManager
{
    constexpr u_int32_t EXP_TIME{60}; // experiment time in seconds
    constexpr u_int32_t PERIOD_MS{1000}; // report period
    constexpr u_int16_t KPM_RAN_FUNC_ID{2};

    // used to check message latency
    u_int64_t get_time_now_us();

    // start KPM Manager
    void start(e2_node_arr_xapp_t const& e2Nodes);
};

#endif // KPMMANAGER
