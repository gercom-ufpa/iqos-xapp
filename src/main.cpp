#include <iostream>
#include <unistd.h>

#include "e42_xapp_api.h"
#include "defer.hpp"
#include "e2Info.hpp"
#include "kpmManager.hpp"
#include "logger.hpp"

#include "rt_nonfinite.h"
#include "runPrediction.h"
#include "runPrediction_terminate.h"
#include "runPrediction_types.h"
#include <stdio.h>

int main(int argc, char* argv[])
{
    // format args
    fr_args_t args{init_fr_args(argc, argv)};

    // init logger
    configureLogger("iqos-xapp", spdlog::level::debug);

    // set FlexRIC IP (just for development)
    //args.ip = {"127.0.0.1"};

    // init xApp
    init_xapp_api(&args);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    // get E2 Nodes
    e2_node_arr_xapp_t e2Nodes{e2_nodes_xapp_api()};
    assert(e2Nodes.len > 0 && "There are no E2 nodes connected!");

    // free memory allocated to E2 Nodes when finished
    defer(free_e2_node_arr_xapp(&e2Nodes));
    ////////////
    SPDLOG_INFO("Inicio do teste do modelo");
    categorical pred;
    cell_wrap_0 r;
    r.f1[0] = 0.421761282626275;
    r.f1[1] = 0.678735154857774;
    r.f1[2] = 0.915735525189067;
    r.f1[3] = 0.757740130578333;
    r.f1[4] = 0.792207329559554	;
    r.f1[5] = 0.743132468124916;
    r.f1[6] = 0.959492426392903;
    r.f1[7] = 0.392227019534168;
    r.f1[8] = 0.655740699156587;
    r.f1[9] = 0.655477890177557;
    runPrediction(&r, &pred);
    //SPDLOG_INFO("id: %d\n", id);
    SPDLOG_INFO("Codes {}", pred.codes);
    //SPDLOG_INFO("id: %d\n", pred.categoryNames);
    SPDLOG_INFO("Final do teste do modelo");
    
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
