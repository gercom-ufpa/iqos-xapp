#include "kafka/producer.hpp"

RdKafka::Conf* KafkaProducer::kafkaConf = nullptr;

void ReportCb::dr_cb(RdKafka::Message& message)
{
    /* If message.err() is non-zero the message delivery failed permanently
    * for the message. */
    if (message.err())
    {
        SPDLOG_ERROR("Message delivery failed: {}", message.errstr());
    }
    else
    {
        SPDLOG_ERROR("Message delivered to topic {} | Partition {} | Offset {}", message.topic_name(),
                     message.partition(), message.offset());
    }
}

void KafkaProducer::set_config(const kconfig &cfg)
{
    if (cfg.empty())
    {
        SPDLOG_ERROR("Kafka configuration is empty.");
    }

    // create a kafka configuration obj
    RdKafka::Conf* conf{RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL)};
    // printable err
    std::string errstr;

    /* Set bootstrap broker(s) as a comma-separated list of
    * host or host:port (default port 9092). */
    if (conf->set("bootstrap.servers", cfg.at("bootstrap.servers"), errstr) != RdKafka::Conf::CONF_OK)
    {
        SPDLOG_ERROR("Error to setup bootstrap servers: {}", errstr);
    }

    // set acks config
    if (conf->set("acks", cfg.at("acks"), errstr) != RdKafka::Conf::CONF_OK)
    {
        SPDLOG_ERROR("Error to setup acks: {}", errstr);
    }

    ReportCb msg_cb;

    // set msg callback config
    if (conf->set("dr_cb", &msg_cb, errstr))
    {
        SPDLOG_ERROR("Error to setup callback function: {}", errstr);
    }

    kafkaConf = conf;
}


void KafkaProducer::send_msg(const std::string &msg, const std::string& topic)
{
    std::string errstr;
    // Create producer instance
    RdKafka::Producer* producer{RdKafka::Producer::create(kafkaConf, errstr)};

    if (!producer)
    {
        SPDLOG_ERROR("Failed to create producer: {}", errstr);
        return;
    }

retry:
    const RdKafka::ErrorCode err{
        producer->produce(
            /* Topic name */
            topic,
            /* Any Partition: the builtin partitioner will be
            * used to assign the message to a topic based
            * on the message key, or random partition if
            * the key is not set. */
            RdKafka::Topic::PARTITION_UA,
            /* Make a copy of the value */
            RdKafka::Producer::RK_MSG_COPY /* Copy payload */,
            /* Value */
            const_cast<char*>(msg.c_str()), msg.length(),
            /* Key */
            nullptr, 0,
            /* Timestamp (defaults to current time) */
            0,
            /* Message headers, if any */
            nullptr,
            /* Per-message opaque value passed to
            * delivery report */
            nullptr
        )
    };


    if (err != RdKafka::ERR_NO_ERROR)
    {
        SPDLOG_ERROR("Failed to produce to topic {}: {}", topic, RdKafka::err2str(err));

        if (err == RdKafka::ERR__QUEUE_FULL)
        {
            /* If the internal queue is full, wait for
            * messages to be delivered and then retry. */
            producer->poll(1000);
            goto retry;
        }
    }
    else
    {
        SPDLOG_INFO("Enqueued message {} for topic {} on Kafka cluster", msg.length(), topic);
    }

    // producer->poll(5);
    SPDLOG_DEBUG("Flushing messages...");
    producer->flush(10); /* wait for max 10 ms */

    if (producer->outq_len() > 0)
    {
        SPDLOG_ERROR("{:d} message(s) were not delivered", producer->outq_len());
    }

    delete producer;
}