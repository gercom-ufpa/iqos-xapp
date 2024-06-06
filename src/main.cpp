#include <iostream>
#include <unistd.h>
#include <db/db.h>
#include <db/sqlite3/sqlite3_wrapper.h>

#include "../libs/flexric/src/xApp/e42_xapp_api.h"
#include "defer.hpp"
#include "e2Info.hpp"
#include "kpmManager.hpp"
#include "logger.hpp"

int main(int argc, char *argv[]) {
    // format args
    fr_args_t args{init_fr_args(argc, argv)};

    // init logger
    configureLogger("IQoS-xApp", spdlog::level::debug);

    // set FlexRIC IP (just for development)
    args.ip = {"192.168.122.10"};

    // init xApp
    init_xapp_api(&args);
    sleep(1);

    // get E2 Nodes
    e2_node_arr_xapp_t e2Nodes{e2_nodes_xapp_api()};
    assert(e2Nodes.len > 0 && "There are no E2 nodes connected!");

    // free memory allocated to E2 Nodes when finished
    defer(free_e2_node_arr_xapp(&e2Nodes));

    // setup db
    // db_xapp_t db {};
    // for (size_t i = 0; i < e2Nodes.len; ++i)
    // {
    //     write_db_sqlite3(db.handler, e2Nodes.n[i].id, )
    // }

    // E2 nodes info
    e2Info::printE2Nodes(e2Nodes);

    // start KPM module
    // KpmManager::start(e2Nodes);


    // wait until all xApp processes have been completed
    while (try_stop_xapp_api() == false) {
        usleep(1000); // 1 ms
    }

    SPDLOG_INFO("xApp finished!");
    return EXIT_SUCCESS;
}
