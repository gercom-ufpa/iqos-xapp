#include <iostream>
#include <unistd.h>

#include "e42_xapp_api.h"
#include "defer.hpp"
#include "e2Info.hpp"
#include "kpmManager.hpp"
#include "logger.hpp"

int main(int argc, char* argv[])
{
    // format args
    fr_args_t args{init_fr_args(argc, argv)};

    // init logger
    configureLogger("iqos-xapp", spdlog::level::debug);

    // set FlexRIC IP (just for development)
    args.ip = {"192.168.100.44"};

    // init xApp
    init_xapp_api(&args);
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // get E2 Nodes
    e2_node_arr_xapp_t e2Nodes{e2_nodes_xapp_api()};
    assert(e2Nodes.len > 0 && "There are no E2 nodes connected!");

    // free memory allocated to E2 Nodes when finished
    defer(free_e2_node_arr_xapp(&e2Nodes));

    // KPM module
    KpmManager::start(e2Nodes);

    // wait until all xApp processes have been completed
    while (try_stop_xapp_api() == false)
    {
        std::this_thread::sleep_for(std::chrono::microseconds(1));
    }

    SPDLOG_INFO("xapp finished!");
    return EXIT_SUCCESS;
}
