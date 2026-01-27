#pragma once

#include <librdkafka/rdkafka.h>

#include <userver/kafka/impl/holders.hpp>

USERVER_NAMESPACE_BEGIN

namespace kafka::impl {

using ErrorHolder = HolderBase<rd_kafka_error_t, &rd_kafka_error_destroy>;
using EventHolder = HolderBase<rd_kafka_event_t, &rd_kafka_event_destroy>;
using QueueHolder = HolderBase<rd_kafka_queue_t, &rd_kafka_queue_destroy>;
using TopicPartitionsListHolder = HolderBase<rd_kafka_topic_partition_list_t, &rd_kafka_topic_partition_list_destroy>;
using MetadataHolder = HolderBase<const rd_kafka_metadata_t, &rd_kafka_metadata_destroy>;
using TopicHolder = HolderBase<rd_kafka_topic_t, &rd_kafka_topic_destroy>;

using ConsumerHolder = KafkaClientHolder<ClientType::kConsumer>;
using ProducerHolder = KafkaClientHolder<ClientType::kProducer>;

}  // namespace kafka::impl

USERVER_NAMESPACE_END
