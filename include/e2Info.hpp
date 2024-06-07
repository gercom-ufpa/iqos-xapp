#ifndef E2INFO_HPP
#define E2INFO_HPP

#include <e2_node_arr_xapp.h>

#include "spdlog/spdlog.h"

#include <iostream>


namespace e2Info
{
    // print E2 nodes informations
    void printE2Nodes(const e2_node_arr_xapp_t& e2Nodes);

    // TODO
    // global_e2_node_id_t getE2NodesIDs(e2_node_arr_xapp_t& e2Nodes);
}


#endif //E2INFO_HPP
