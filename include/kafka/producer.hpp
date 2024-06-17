//
// Created by murilo on 14/06/24.
//

#ifndef PRODUCER_HPP
#define PRODUCER_HPP

#include <map>

#include "librdkafka/rdkafkacpp.h"
#include "spdlog/spdlog.h"

class KafkaProducer
{
    static RdKafka::Conf* kafkaConf;

public:
    using kconfig = std::map<std::string, std::string>;

    // creates a Kafka cluster config
    static void set_config(const kconfig& cfg);

    // send message to Kafka cluster
    static void send_msg(const std::string &msg, const std::string& topic);
};

// Class to handle message callbacks
class ReportCb final : public RdKafka::DeliveryReportCb
{
public:
    void dr_cb(RdKafka::Message& message) override;
};


#endif //PRODUCER_HPP
