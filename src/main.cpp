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
    configureLogger("IQoS-xApp", spdlog::level::debug);

    // set FlexRIC IP (just for development)
    // args.ip = {"192.168.122.10"};

    // init xApp
    init_xapp_api(&args);
    sleep(1);

    // get E2 Nodes
    e2_node_arr_xapp_t e2Nodes{e2_nodes_xapp_api()};
    assert(e2Nodes.len > 0 && "There are no E2 nodes connected!");

    // free memory allocated to E2 Nodes when finished
    defer(free_e2_node_arr_xapp(&e2Nodes));

    // E2Info module
    auto e2Info {E2Info(e2Nodes)};
    e2Info.printE2Nodes();

    // KPM module
    // KpmManager::start(e2Nodes);

    // wait until all xApp processes have been completed
    while (try_stop_xapp_api() == false)
    {
        usleep(1000); // 1 ms
    }

    SPDLOG_INFO("xApp finished!");
    return EXIT_SUCCESS;
}
