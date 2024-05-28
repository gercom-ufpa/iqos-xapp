#include <iostream>
#include <unistd.h>

#include "../libs/flexric/src/xApp/e42_xapp_api.h"
#include "defer.hpp"

int main(int argc, char *argv[]) {
    // format args
    fr_args_t args{init_fr_args(argc, argv)};

    // set FlexRIC IP (just for development)
    args.ip = {"192.168.122.20"};

    // init xApp
    init_xapp_api(&args);
    sleep(1);

    // get E2 Nodes
    e2_node_arr_xapp_t e2Nodes{e2_nodes_xapp_api()};
    assert(e2Nodes.len > 0 && "There are no E2 nodes connected!");

    // free memory allocated to E2 Nodes when finished
    defer(free_e2_node_arr_xapp(&e2Nodes));

    // TODO: print E2 nodes infos
    std::cout << "There are " << static_cast<unsigned>(e2Nodes.len) << " E2 nodes connected" << '\n';

    // wait until all xApp processes have been completed
    while (try_stop_xapp_api() == false) {
        usleep(1000); // 1 ms
    }

    std::cout << "Hello, FlexRIC :)!" << '\n';
    return EXIT_SUCCESS;
}
