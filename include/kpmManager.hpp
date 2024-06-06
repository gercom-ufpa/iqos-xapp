#ifndef KPMMANAGER
#define KPMMANAGER

#include "../libs/flexric/src/xApp/e42_xapp_api.h"
#include "spdlog/spdlog.h"
#include "defer.hpp"

#include "chrono"
#include <cassert>

namespace KpmManager
{
    // used to check message latency
    u_int64_t get_time_now_us();

    // start KPM Manager
    void start(e2_node_arr_xapp_t const& e2Nodes);
};

#endif // KPMMANAGER
