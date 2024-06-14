#ifndef E2INFO_HPP
#define E2INFO_HPP


#include <e2_node_arr_xapp.h>

#include "spdlog/spdlog.h"
#include <tabulate/table.hpp>

#include <iostream>

class E2Info
{
public:
    using e2NodesIdLst = std::vector<global_e2_node_id_t>;
    e2_node_arr_xapp_t e2Nodes{};

    explicit E2Info(e2_node_arr_xapp_t &e2Nodes);

    void printE2Nodes() const;
    [[nodiscard]] e2NodesIdLst getE2NodesIds() const;
};


#endif //E2INFO_HPP
