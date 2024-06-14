#include "e2Info.hpp"

E2Info::E2Info(e2_node_arr_xapp_t& e2Nodes) : e2Nodes(e2Nodes)
{}

void E2Info::printE2Nodes() const
{
    SPDLOG_INFO("Showing E2 node informations...");

    // creates tables with E2 nodes infos
    // table header
    tabulate::Table e2NodesInfoTabHdr;
    e2NodesInfoTabHdr.add_row({"Information of connected E2 nodes"});
    // format table header
    e2NodesInfoTabHdr.format()
                     .column_separator("")
                     .border_top("\u2014")
                     .border_bottom("\u2014");
    e2NodesInfoTabHdr[0].format()
                        .font_align(tabulate::FontAlign::center);

    // information table
    tabulate::Table e2NodesInfoTab;
    e2NodesInfoTab.add_row({
        "index", "nb_id", "mcc", "mnc", "mnc_digit_len", "ran_type", "cu_du_id", "supported rf_ids"
    });

    for (size_t i = 0; i < this->e2Nodes.len; ++i)
    {
        const ngran_node_t ran_type = this->e2Nodes.n[i].id.type;
        e2NodesInfoTab.add_row(
            tabulate::RowStream{} <<
            i <<
            this->e2Nodes.n[i].id.nb_id.nb_id <<
            this->e2Nodes.n[i].id.plmn.mcc <<
            this->e2Nodes.n[i].id.plmn.mnc <<
            static_cast<unsigned>(this->e2Nodes.n[i].id.plmn.mnc_digit_len) <<
            get_ngran_name(ran_type)
        );

        // FIXME: segmentation fault to show CU's cu_du_id
        if (!NODE_IS_MONOLITHIC(ran_type) && !NODE_IS_CU(ran_type))
        {
            e2NodesInfoTab.row(i + 1).cell(6).set_text(fmt::to_string(*this->e2Nodes.n[i].id.cu_du_id));
        }

        // print supported RAN Functions
        std::string rf_id_lst;
        for (size_t j = 0; j < this->e2Nodes.n[i].len_rf; ++j)
        {
            // TODO: print RAN Function name
            rf_id_lst.append(fmt::to_string(this->e2Nodes.n[i].rf[j].id) + ' ');
        }

        e2NodesInfoTab.row(i + 1).cell(7).set_text(rf_id_lst);
    }

    // format information table
    e2NodesInfoTab.format()
                  .column_separator("");
    e2NodesInfoTab[0].format()
                     .font_align(tabulate::FontAlign::center)
                     .font_style({tabulate::FontStyle::bold});

    e2NodesInfoTabHdr.add_row(tabulate::RowStream{} << e2NodesInfoTab);

    std::cout << e2NodesInfoTabHdr << '\n';
}

E2Info::e2NodesIdLst E2Info::getE2NodesIds() const
{
    e2NodesIdLst ids{};
    for (size_t i = 0; i < this->e2Nodes.len; ++i)
    {
        ids.push_back(this->e2Nodes.n[i].id);
    }
    return ids;
}
