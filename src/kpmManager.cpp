#include "kpmManager.hpp"

#include <iostream>

namespace KpmManager
{
    static constexpr u_int32_t PERIOD_MS{1000}; // report period
    static constexpr u_int32_t EXP_TIME{30}; // experiment time in seconds
    static constexpr u_int16_t KPM_RAN_FUNC_ID{2};

    static pthread_mutex_t mtx;

    // type to filter and handle with report styles [function()]
    using fill_kpm_act_def = kpm_act_def_t (*)(ric_report_style_item_t const* report_item);

    // report styles functions (implementations below)
    static kpm_act_def_t fill_report_style_4(ric_report_style_item_t const* report_item);

    // select report style by idx
    static fill_kpm_act_def get_kpm_act_def[END_RIC_SERVICE_REPORT]
    {
        nullptr,
        nullptr,
        nullptr,
        fill_report_style_4,
        nullptr,
    };

    // type to filter and handle with meas_type [function()]
    using check_meas_type = void (*)(meas_type_t const& meas_type, meas_record_lst_t const& meas_record);

    // meas type functions (implementations below)
    static void match_meas_name_type(meas_type_t const& meas_type, meas_record_lst_t const& meas_record);
    static void match_meas_type_id(meas_type_t const& meas_type, meas_record_lst_t const& meas_record);

    // select meas_type by idx
    static check_meas_type match_meas_type[meas_type_t::END_MEAS_TYPE]
    {
        match_meas_name_type, // 8.3.9 in O-RAN spec
        match_meas_type_id, // 8.3.10 in O-RAN spec
    };

    // type to filter and handle with meas_value [function()]
    using handle_meas_value = void (*)(byte_array_t name, meas_record_lst_t meas_record);

    // functions to handle meas value (implementations below)
    static void int_meas_value(byte_array_t name, meas_record_lst_t meas_record);
    static void real_meas_value(byte_array_t name, meas_record_lst_t meas_record);

    static handle_meas_value get_meas_value[END_MEAS_VALUE]
    {
        int_meas_value,
        real_meas_value,
        nullptr, // no value
    };

    // convert a type byte_array_t to string
    static std::string ba_to_string(byte_array_t const& byteArray)
    {
        return std::string{reinterpret_cast<const char*>(byteArray.buf), byteArray.len};
    }

    static void int_meas_value(byte_array_t name, meas_record_lst_t meas_record)
    {
        // std::unordered_map<std::string, std::function<void()>> actions;
        std::string meas_name{ba_to_string(name)};

        // match meas by name (TODO: review this)
        if (meas_name == "RRU.PrbTotDl")
        {
            SPDLOG_DEBUG("DRB.PrbTotDl = {:d} [PRBs]", meas_record.int_val);
        }
        else if (meas_name == "RRU.PrbTotUl")
        {
            SPDLOG_DEBUG("DRB.PrbTotUl = {:d} [PRBs]", meas_record.int_val);
        }
        else if (meas_name == "DRB.PdcpSduVolumeDL")
        {
            SPDLOG_DEBUG("DRB.PdcpSduVolumeDL = {:d} [Mbits]", meas_record.int_val);
        }
        else if (meas_name == "DRB.PdcpSduVolumeUL")
        {
            SPDLOG_DEBUG("DRB.PdcpSduVolumeUL = {:d} [Mbits]", meas_record.int_val);
        }
        else if (meas_name == "DRB.RlcPacketDropRateDl") // is int??
        {
            SPDLOG_DEBUG("DRB.RlcPacketDropRateDl = {:d} [%]", meas_record.int_val);
        }
        else if (meas_name == "DRB.PacketSuccessRateUlgNBUu") // is int??
        {
            SPDLOG_DEBUG("DRB.PacketSuccessRateUlgNBUu = {:d} [%]", meas_record.int_val);
        }
        else
        {
            // SPDLOG_DEBUG("Measurement {} not yet supported", meas_name);
        }
    }

    static void real_meas_value(byte_array_t name, meas_record_lst_t meas_record)
    {
        std::string meas_name{ba_to_string(name)};

        // match meas by name
        if (meas_name == "DRB.UEThpDl")
        {
            SPDLOG_DEBUG("DRB.UEThpDl = {:.2f} [Kbit/s]", meas_record.real_val);
        }
        else if (meas_name == "DRB.UEThpUl")
        {
            SPDLOG_DEBUG("DRB.UEThpUl = {:.2f} [Kbit/s]", meas_record.real_val);
        }
        else if (meas_name == "DRB.RlcSduDelayDl")
        {
            SPDLOG_DEBUG("DRB.RlcSduDelayDl = {:.2f} [μs]", meas_record.real_val);
        }
        else
        {
            // SPDLOG_DEBUG("Measurement {} not yet supported", meas_name);
        }
    }

    // print UE id type
    static void print_ue_id(ue_id_e2sm_e type, ue_id_e2sm_t ue_id)
    {
        switch (type) // define UE ID type (only 5G)
        {
        case ue_id_e2sm_e::GNB_UE_ID_E2SM:
            if (ue_id.gnb.gnb_cu_ue_f1ap_lst != nullptr)
            {
                for (size_t i = 0; i < ue_id.gnb.gnb_cu_ue_f1ap_lst_len; ++i)
                {
                    // split CU
                    SPDLOG_DEBUG("UE ID type = gNB-CU | gnb_cu_ue_f1ap = {:d}", ue_id.gnb.gnb_cu_ue_f1ap_lst[i]);
                }
            }
            else
            {
                SPDLOG_DEBUG("UE ID type = gNB | amf_ue_ngap_id = {:d}", ue_id.gnb.amf_ue_ngap_id); // Monolitic ID
            }

            if (ue_id.gnb.ran_ue_id != nullptr)
            {
                SPDLOG_DEBUG("ran_ue_id = {:x}", *ue_id.gnb.ran_ue_id); // RAN UE NGAP ID
            }
            break;
        case ue_id_e2sm_e::GNB_DU_UE_ID_E2SM:
            SPDLOG_DEBUG("UE ID type = gNB-DU | gnb_cu_ue_f1ap = {:d}", ue_id.gnb_du.gnb_cu_ue_f1ap);
            if (ue_id.gnb_du.ran_ue_id != nullptr)
            {
                SPDLOG_DEBUG("ran_ue_id = {:x}", *ue_id.gnb_du.ran_ue_id); // RAN UE NGAP ID
            }
            break;
        case ue_id_e2sm_e::GNB_CU_UP_UE_ID_E2SM:
            SPDLOG_DEBUG("UE ID type = gNB-CU-UP | gnb_cu_cp_ue_e1ap = {:d}", ue_id.gnb_cu_up.gnb_cu_cp_ue_e1ap);
            if (ue_id.gnb_cu_up.ran_ue_id != nullptr)
            {
                SPDLOG_DEBUG("ran_ue_id = {:x}", *ue_id.gnb_cu_up.ran_ue_id); // RAN UE NGAP ID
            }
            break;
        default:
            SPDLOG_WARN("UE ID type not supported!");
            break;
        }
    }

    // print UE measurements
    static void print_measurements(kpm_ind_msg_format_1_t const* msg_frm_1)
    {
        assert(msg_frm_1->meas_info_lst_len > 0 && "Cannot correctly print measurements");

        // UE Measurements per granularity period
        for (size_t i = 0; i < msg_frm_1->meas_data_lst_len; ++i)
        {
            meas_data_lst_t const data_item{msg_frm_1->meas_data_lst[i]}; // get item
            for (int j = 0; j < data_item.meas_record_len; ++j)
            {
                const meas_type_t meas_type{msg_frm_1->meas_info_lst[j].meas_type}; // item type
                const meas_record_lst_t record_item{data_item.meas_record_lst[j]};

                match_meas_type[meas_type.type](meas_type, record_item);

                if (data_item.incomplete_flag && *data_item.incomplete_flag == TRUE_ENUM_VALUE)
                {
                    SPDLOG_DEBUG("Measurement Record not reliable");
                }
            }
        }
    }

    static size_t get_kpm_idx(sm_ran_function_t const* rf, size_t rf_size)
    {
        for (size_t i = 0; i <= rf_size; i++)
        {
            if (rf[i].id == KPM_RAN_FUNC_ID)
            {
                return i;
            }
        }

        assert(0 != 0 && "KPM SM ID could not be found in the RAN Function List");
    }

    // return a filled test info lst
    static test_info_lst_t filter_predicate()
    {
        test_info_lst_t dst{};

        // filter by S-NSSAI criteria
        constexpr test_cond_type_e type{S_NSSAI_TEST_COND_TYPE};
        constexpr test_cond_e condition{EQUAL_TEST_COND};
        constexpr u_int8_t value{1};

        dst.test_cond_type = {type};
        dst.S_NSSAI = {TRUE_TEST_COND_TYPE};

        // alloc test cond
        dst.test_cond = {static_cast<test_cond_e*>(calloc(1, sizeof(test_cond_e)))};
        assert(dst.test_cond != nullptr && "Memory exhausted!");
        *dst.test_cond = {condition};

        // alloc test cond type
        dst.test_cond_value = {static_cast<test_cond_value_t*>(calloc(1, sizeof(test_cond_value_t)))};
        assert(dst.test_cond_value != nullptr && "Memory exhausted!");
        dst.test_cond_value->type = {OCTET_STRING_TEST_COND_VALUE};

        // alloc octet string
        dst.test_cond_value->octet_string_value = {static_cast<byte_array_t*>(calloc(1, sizeof(byte_array_t)))};
        assert(dst.test_cond_value->octet_string_value != nullptr && "Memory exhausted!");

        constexpr size_t len_nssai{1};
        dst.test_cond_value->octet_string_value->len = {len_nssai};

        // alloc octet buffer
        dst.test_cond_value->octet_string_value->buf = {static_cast<uint8_t*>(calloc(len_nssai, sizeof(uint8_t)))};
        assert(dst.test_cond_value->octet_string_value->buf != nullptr && "Memory exhausted!");
        dst.test_cond_value->octet_string_value->buf[0] = value;

        return dst;
    }

    static label_info_lst_t fill_kpm_label()
    {
        label_info_lst_t label_item{nullptr};

        label_item.noLabel = {static_cast<enum_value_e*>(calloc(1, sizeof(enum_value_e)))};
        *label_item.noLabel = {TRUE_ENUM_VALUE};

        return label_item;
    }

    // return a type 1 definition format
    static kpm_act_def_format_1_t fill_act_def_frm_1(ric_report_style_item_t const* report_item)
    {
        assert(report_item != nullptr);
        kpm_act_def_format_1_t ad_frm_1{0};

        const size_t size{report_item->meas_info_for_action_lst_len};

        // [1, 65535]
        ad_frm_1.meas_info_lst_len = size;
        ad_frm_1.meas_info_lst = static_cast<meas_info_format_1_lst_t*>(calloc(size, sizeof(meas_info_format_1_lst_t)));
        assert(ad_frm_1.meas_info_lst != nullptr && "Memory exhausted!");

        // iterate over each measure
        for (size_t i = 0; i < size; i++)
        {
            meas_info_format_1_lst_t* meas_item{&ad_frm_1.meas_info_lst[i]};

            // Measurement Name
            // 8.3.9 in the O-RAN specification
            meas_item->meas_type.type = {meas_type_t::NAME_MEAS_TYPE};
            meas_item->meas_type.name = {copy_byte_array(report_item->meas_info_for_action_lst[i].name)};

            // Measurement Label
            // 8.3.11 in the O-RAN specification
            meas_item->label_info_lst_len = {1};
            meas_item->label_info_lst = static_cast<label_info_lst_t*>(calloc(1, sizeof(label_info_lst_t)));
            assert(meas_item->label_info_lst != nullptr && "Memory exhausted!");
            meas_item->label_info_lst[0] = fill_kpm_label();
        }

        // Granularity Period
        // 8.3.8 in the O-RAN specification
        ad_frm_1.gran_period_ms = {PERIOD_MS};

        // Cell Global ID
        // 8.3.20 in the O-RAN specification - OPTIONAL
        ad_frm_1.cell_global_id = {nullptr};

#if defined KPM_V2_03 || defined KPM_V3_00
        // [0, 65535]
        ad_frm_1.meas_bin_range_info_lst_len = {0};
        ad_frm_1.meas_bin_info_lst = {nullptr};
#endif

        return ad_frm_1;
    }

    u_int64_t get_time_now_us()
    {
        auto now{std::chrono::high_resolution_clock::now()};
        auto duration{std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch())};
        return duration.count();
    }

    // callback to handle return data from KPM subscription
    static void sm_cb_kpm(sm_ag_if_rd_t const* rd)
    {
        assert(rd != nullptr);
        assert(rd->type == INDICATION_MSG_AGENT_IF_ANS_V0);
        assert(rd->ind.type == KPM_STATS_V3_0);

        // Reading Indication Message Format 3
        // 8.2.1.4.3 in the O-RAN specification
        kpm_ind_data_t const* ind{&rd->ind.kpm.ind}; // indication
        kpm_ric_ind_hdr_format_1_t const* hdr_frm_1{&ind->hdr.kpm_ric_ind_hdr_format_1}; // ind header
        kpm_ind_msg_format_3_t const* msg_frm_3{&ind->msg.frm_3}; // ind message

        static int counter = 1;

        // create a new scope
        {
            u_int64_t now{get_time_now_us()};
            pthread_mutex_lock(&mtx);
            defer(pthread_mutex_unlock(&mtx));


            // print latency xApp <-> E2 Node
            std::cout << "\n--------------------------------------[KPM Message " << counter << " | Latency " << now -
                hdr_frm_1->collectStartTime << " (μs)]--------------------------------------" << '\n';
            // SPDLOG_DEBUG("KPM ind_msg latency = {:d} [μs]",);

            // Reported list of measurements per UE
            for (size_t i = 0; i < msg_frm_3->ue_meas_report_lst_len; ++i)
            {
                // get UE ID
                const ue_id_e2sm_t ue_id = msg_frm_3->meas_report_per_ue[i].ue_meas_report_lst; // id
                const ue_id_e2sm_e ue_id_type = ue_id.type; // type
                print_ue_id(ue_id_type, ue_id);

                // print measurements
                print_measurements(&msg_frm_3->meas_report_per_ue[i].ind_msg_format_1);
            }

            counter++;
        }
    }

    // handle with style report type 4
    static kpm_act_def_t fill_report_style_4(ric_report_style_item_t const* report_item)
    {
        assert(report_item != nullptr);
        assert(report_item->act_def_format_type == FORMAT_4_ACTION_DEFINITION);

        // set kpm_act type to 4
        kpm_act_def_t act_def{.type = FORMAT_4_ACTION_DEFINITION};

        // Fill matching condition
        // [1, 32768]
        act_def.frm_4.matching_cond_lst_len = 1;
        act_def.frm_4.matching_cond_lst = static_cast<matching_condition_format_4_lst_t*>(calloc(
            act_def.frm_4.matching_cond_lst_len, sizeof(matching_condition_format_4_lst_t)));
        assert(act_def.frm_4.matching_cond_lst != nullptr && "Memory exhausted");

        // Filter connected UEs by some criteria
        act_def.frm_4.matching_cond_lst[0].test_info_lst = {filter_predicate()};

        // Fill Action Definition Format 1
        // 8.2.1.2.1 in the O-RAN spec
        act_def.frm_4.action_def_format_1 = fill_act_def_frm_1(report_item);

        return act_def;
    }

    // handle with meansurement name
    static void match_meas_name_type(meas_type_t const& meas_type, meas_record_lst_t const& meas_record)
    {
        get_meas_value[meas_record.value](meas_type.name, meas_record);
    }

    // TODO: not implemented yet
    static void match_meas_type_id(meas_type_t const& meas_type, meas_record_lst_t const& meas_record)
    {
        (void)meas_type;
        (void)meas_record;
        assert(false && "ID Measurement Type not yet supported");
    }

    static kpm_sub_data_t gen_kpm_subs(kpm_ran_function_def_t const* ran_func)
    {
        assert(ran_func != nullptr);
        assert(ran_func->ric_event_trigger_style_list != nullptr);

        kpm_sub_data_t kpm_sub{};

        // Generate Event Trigger
        assert(ran_func->ric_event_trigger_style_list[0].format_type == FORMAT_1_RIC_EVENT_TRIGGER);
        kpm_sub.ev_trg_def.type = FORMAT_1_RIC_EVENT_TRIGGER;
        kpm_sub.ev_trg_def.kpm_ric_event_trigger_format_1.report_period_ms = PERIOD_MS;

        // Generate Action Definition
        kpm_sub.sz_ad = 1;
        kpm_sub.ad = static_cast<kpm_act_def_t*>(calloc(kpm_sub.sz_ad, sizeof(kpm_act_def_t)));
        assert(kpm_sub.ad != nullptr && "Memory exhausted");

        // Multiple Action Definitions in one SUBSCRIPTION message is not supported
        // Multiple REPORT Styles = Multiple Action Definition = Multiple SUBSCRIPTION messages
        ric_report_style_item_t* const report_item = &ran_func->ric_report_style_list[0];
        ric_service_report_e const report_style_type = report_item->report_style_type;
        *kpm_sub.ad = get_kpm_act_def[report_style_type](report_item);

        return kpm_sub;
    }


    void start(e2_node_arr_xapp_t const& e2Nodes)
    {
        SPDLOG_INFO("Starting KPM module...");
        // configure mutex
        pthread_mutexattr_t attr{0};
        int rc{pthread_mutex_init(&mtx, &attr)};
        assert(rc == 0);

        // alloc memory for reports
        auto hndl{static_cast<sm_ans_xapp_t*>(calloc(e2Nodes.len, sizeof(sm_ans_xapp_t)))};
        assert(hndl != nullptr);
        defer(free(hndl));

        for (size_t i = 0; i < e2Nodes.len; ++i)
        {
            e2_node_connected_xapp_t* e2Node{&e2Nodes.n[i]}; // get E2 node
            const size_t kpm_idx{get_kpm_idx(e2Node->rf, e2Node->len_rf)}; // get KPM idx in list
            assert(e2Node->rf[kpm_idx].defn.type == KPM_RAN_FUNC_DEF_E && "KPM is not the received RAN Function");

            // if REPORT Service is supported by E2 node, send SUBSCRIPTION
            // e.g. OAI CU-CP
            if (e2Node->rf[kpm_idx].defn.kpm.ric_report_style_list != nullptr)
            {
                // Generate KPM SUBSCRIPTION message
                kpm_sub_data_t kpm_sub = gen_kpm_subs(&e2Node->rf[kpm_idx].defn.kpm);

                // use API to send subscription
                hndl[i] = {report_sm_xapp_api(&e2Node->id, KPM_RAN_FUNC_ID, &kpm_sub, sm_cb_kpm)};
                assert(hndl[i].success == true);
                free_kpm_sub_data(&kpm_sub);
            }
        }

        // time to received metrics
        sleep(EXP_TIME);

        for (size_t i = 0; i < e2Nodes.len; ++i)
        {
            // Remove the handle previously returned
            if (hndl[i].success == true)
            {
                rm_report_sm_xapp_api(hndl[i].u.handle);
            }
        }
    }
}
