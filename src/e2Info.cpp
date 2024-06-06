#include "e2Info.hpp"

namespace e2Info
{
    void printE2Nodes(const e2_node_arr_xapp_t& e2Nodes)
    {
        SPDLOG_INFO("There are {} E2 nodes connected", static_cast<unsigned>(e2Nodes.len));

        for (size_t i = 0; i < e2Nodes.len; ++i)
        {
            const ngran_node_t ran_type = e2Nodes.n[i].id.type;
            SPDLOG_INFO("E2 node {:d} info: nb_id {:d} | mcc {:d} | mnc {:d} | mnc_digit_len {:d} | ran_type {}",
                         i, e2Nodes.n[i].id.nb_id.nb_id, e2Nodes.n[i].id.plmn.mcc,
                         e2Nodes.n[i].id.plmn.mnc,
                         e2Nodes.n[i].id.plmn.mnc_digit_len,
                         get_ngran_name(ran_type));

            if (!NODE_IS_MONOLITHIC(ran_type) && !NODE_IS_CU(ran_type)) // FIXME: xApp fails to show CU's cu_du_id
            {
                SPDLOG_INFO("E2 node {:d} info: cu_du_id {:d}", i, *e2Nodes.n[i].id.cu_du_id);
            }

            // print supported RAN Functions
            for (size_t j = 0; j < e2Nodes.n[i].len_rf; ++j)
            {
                // TODO: print RAN Function name
                SPDLOG_INFO("E2 node {:d} info: supported RAN function ID {:d}", i, e2Nodes.n[i].rf[j].id);
            }
        }
    }
}
