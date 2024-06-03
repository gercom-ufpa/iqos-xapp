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

    // type to filter and handle with report styles
    using fill_kpm_act_def = kpm_act_def_t (*)(ric_report_style_item_t const* report_item);

    // start KPM Manager
    void start(e2_node_arr_xapp_t const& e2Nodes);
};

#endif // KPMMANAGER
