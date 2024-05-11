#include <kafka/impl/consumer_impl.hpp>

#include <chrono>

#include <userver/logging/log.hpp>
#include <userver/testsuite/testpoint.hpp>
#include <userver/utils/span.hpp>

#include <kafka/impl/configuration.hpp>
#include <kafka/impl/error_buffer.hpp>
#include <kafka/impl/stats.hpp>

#include <librdkafka/rdkafka.h>

#include <fmt/format.h>

USERVER_NAMESPACE_BEGIN

namespace kafka {

namespace {

using MessageHolder =
    std::unique_ptr<rd_kafka_message_t, decltype(&rd_kafka_message_destroy)>;

std::optional<std::chrono::milliseconds> RetrieveTimestamp(
    const rd_kafka_message_t* message) {
  rd_kafka_timestamp_type_t type = RD_KAFKA_TIMESTAMP_NOT_AVAILABLE;
  const std::int64_t timestamp = rd_kafka_message_timestamp(message, &type);
  if (timestamp == -1 || type == RD_KAFKA_TIMESTAMP_NOT_AVAILABLE) {
    return std::nullopt;
  }

  return std::chrono::milliseconds{timestamp};
}

}  // namespace

struct Message::Data final {
  Data(MessageHolder message_holder)
      : message(std::move(message_holder)),
        topic(rd_kafka_topic_name(message->rkt)),
        timestamp(RetrieveTimestamp(message.get())) {}

  Data(Data&& other) noexcept
      : message(std::move(other.message)),
        topic(rd_kafka_topic_name(message->rkt)),
        timestamp(RetrieveTimestamp(message.get())) {}

  MessageHolder message;

  const std::string topic;
  std::optional<std::chrono::milliseconds> timestamp;
};

Message::~Message() = default;

Message::Message(Message&& other) noexcept = default;

const std::string& Message::GetTopic() const { return data_->topic; }

std::string_view Message::GetKey() const {
  const auto* key = data_->message->key;

  if (!key) {
    return std::string_view{};
  }

  return std::string_view{static_cast<const char*>(key),
                          data_->message->key_len};
}

std::string_view Message::GetPayload() const {
  return std::string_view{static_cast<const char*>(data_->message->payload),
                          data_->message->len};
}

std::optional<std::chrono::milliseconds> Message::GetTimestamp() const {
  return data_->timestamp;
}

int Message::GetPartition() const { return data_->message->partition; }

std::int64_t Message::GetOffset() const { return data_->message->offset; }

Message::Message(Message::DataStorage data) : data_(std::move(data)) {}

namespace impl {

namespace {

void PrintTopicPartitionsList(
    const rd_kafka_topic_partition_list_t* list,
    const logging::LogExtra& log_tags,
    std::function<std::string(const rd_kafka_topic_partition_t&)> log,
    bool skip_invalid_offsets = false) {
  if (list == nullptr || list->cnt == 0) {
    return;
  }

  utils::span<const rd_kafka_topic_partition_t> topic_partitions{
      list->elems, list->elems + list->cnt};
  for (const auto& topic_partition : topic_partitions) {
    if (skip_invalid_offsets &&
        topic_partition.offset == RD_KAFKA_OFFSET_INVALID) {
      /// @note `librdkafka` does not sets offsets for partitions that were not
      /// committed in the current in the current commit
      LOG_INFO() << log_tags << "Skipping partition "
                 << topic_partition.partition;
      continue;
    }

    LOG_INFO() << log_tags << log(topic_partition);
  }
}

void CallTestpoints(const rd_kafka_topic_partition_list_t* list,
                    const std::string& testpoint_name) {
  if (list == nullptr || list->cnt == 0 ||
      !testsuite::AreTestpointsAvailable()) {
    return;
  }

  for (int i = 0; i < list->cnt; ++i) {
    TESTPOINT(testpoint_name, {});
  }
}

/// @brief Assigns (subscribes) the @param partitions list to the current
/// consumer.
void AssignPartitions(rd_kafka_t* consumer,
                      const rd_kafka_topic_partition_list_t* partitions,
                      const std::string& consumer_name,
                      const logging::LogExtra& log_tags) {
  LOG_INFO() << log_tags << "Assigning new partitions to consumer";
  PrintTopicPartitionsList(
      partitions, log_tags, [](const rd_kafka_topic_partition_t& partition) {
        return fmt::format("Partition {} for topic '{}' assigning",
                           partition.partition, partition.topic);
      });

  const auto assign_err = rd_kafka_assign(consumer, partitions);
  if (assign_err != RD_KAFKA_RESP_ERR_NO_ERROR) {
    LOG_ERROR() << log_tags
                << fmt::format("Failed to assign partitions: {}",
                               rd_kafka_err2str(assign_err));
    return;
  }

  LOG_INFO() << log_tags << "Successfully assigned partitions";
  CallTestpoints(partitions, fmt::format("tp_{}_subscribed", consumer_name));
}

/// @brief Revokes @param partitions from the current consumer.
void RevokePartitions(rd_kafka_t* consumer,
                      const rd_kafka_topic_partition_list_t* partitions,
                      const std::string& consumer_name,
                      const logging::LogExtra& log_tags) {
  LOG_INFO() << log_tags << "Revoking existing partitions from consumer";

  PrintTopicPartitionsList(
      partitions, log_tags, [](const rd_kafka_topic_partition_t& partition) {
        return fmt::format("Partition {} of '{}' topic revoking",
                           partition.partition, partition.topic);
      });

  const auto revokation_err = rd_kafka_assign(consumer, nullptr);
  if (revokation_err != RD_KAFKA_RESP_ERR_NO_ERROR) {
    LOG_ERROR() << fmt::format("Failed to revoke partitions: {}",
                               rd_kafka_err2str(revokation_err));
    return;
  }

  LOG_INFO() << log_tags << "Successfully revoked partitions";
  CallTestpoints(partitions, fmt::format("tp_{}_revoked", consumer_name));
}

/// @brief Callback that is called on each group join/leave and topic partition
/// update.
void RebalanceCallback(rd_kafka_t* consumer, rd_kafka_resp_err_t err,
                       rd_kafka_topic_partition_list_t* partitions,
                       void* opaque_ptr) {
  const auto& opaque = Opaque::FromPtr(opaque_ptr);
  const auto& consumer_name = opaque.GetComponentName();
  const auto log_tags = opaque.MakeCallbackLogTags("rebalance_callback");

  LOG_INFO() << log_tags
             << fmt::format("Consumer group rebalanced ('{}' protocol)",
                            rd_kafka_rebalance_protocol(consumer));

  switch (err) {
    case RD_KAFKA_RESP_ERR__ASSIGN_PARTITIONS:
      AssignPartitions(consumer, partitions, consumer_name, log_tags);
      break;
    case RD_KAFKA_RESP_ERR__REVOKE_PARTITIONS:
      RevokePartitions(consumer, partitions, consumer_name, log_tags);
      break;
    default:
      LOG_ERROR() << log_tags
                  << fmt::format("Failed when rebalancing: {}",
                                 rd_kafka_err2str(err));
      break;
  }
}

/// @brief Callback which is callbed after succeeded/failed commit.
/// Currently, used for logging purposes.
void OffsetCommitCallback([[maybe_unused]] rd_kafka_t* consumer,
                          rd_kafka_resp_err_t err,
                          rd_kafka_topic_partition_list_t* committed_offsets,
                          void* opaque_ptr) {
  const auto& opaque = Opaque::FromPtr(opaque_ptr);
  const auto log_tags = opaque.MakeCallbackLogTags("offset_commit_callback");

  if (err != RD_KAFKA_RESP_ERR_NO_ERROR) {
    LOG_ERROR() << log_tags
                << fmt::format("Failed to commit offsets: {}",
                               rd_kafka_err2str(err));
    return;
  }

  LOG_INFO() << log_tags << "Successfully committed offsets";
  PrintTopicPartitionsList(
      committed_offsets, log_tags,
      [](const rd_kafka_topic_partition_t& offset) {
        return fmt::format(
            "Offset {} committed for topic '{}' within partition {}",
            offset.offset, offset.topic, offset.partition);
      },
      /*skip_invalid_offsets=*/true);
}

}  // namespace

using TopicPartitionsListHolder =
    std::unique_ptr<rd_kafka_topic_partition_list_t,
                    decltype(&rd_kafka_topic_partition_list_destroy)>;

class ConsumerImpl::ConfHolder final {
 public:
  using HandleHolder =
      std::unique_ptr<rd_kafka_conf_t, decltype(&rd_kafka_conf_destroy)>;

  ConfHolder(void* conf)
      : handle_(static_cast<rd_kafka_conf_t*>(conf), &rd_kafka_conf_destroy) {}

  ConfHolder(const ConfHolder&) = delete;
  ConfHolder& operator=(const ConfHolder&) = delete;

  ConfHolder(ConfHolder&&) noexcept = delete;
  ConfHolder& operator=(ConfHolder&&) noexcept = delete;

  HandleHolder MakeConfCopy() const {
    return {rd_kafka_conf_dup(handle_.get()), &rd_kafka_conf_destroy};
  }

 private:
  HandleHolder handle_;
};

class ConsumerImpl::ConsumerHolder final {
 public:
  ConsumerHolder(ConfHolder::HandleHolder conf_handle, ErrorBuffer& err_buf)
      : handle_(rd_kafka_new(RD_KAFKA_CONSUMER, conf_handle.get(),
                             err_buf.data(), err_buf.size()),
                &rd_kafka_destroy) {
    if (handle_ == nullptr) {
      /// @note `librdkafka` takes ownership on conf iff `rd_kafka_new`
      /// succeeds

      PrintErrorAndThrow("create consumer", err_buf);
    }
    /// @note `rd_kafka_new` takes an ownership on config, if succeeds
    [[maybe_unused]] auto _ = conf_handle.release();

    /// @note makes available to call `rd_kafka_consumer_poll`
    rd_kafka_poll_set_consumer(Handle());
  }

  ~ConsumerHolder() {
    if (handle_ != nullptr) {
      rd_kafka_consumer_close(Handle());
    }
    LOG_INFO() << "Consumer closed";
  }

  ConsumerHolder(const ConsumerHolder&) = delete;
  ConsumerHolder& operator=(const ConsumerHolder&) = delete;

  ConsumerHolder(ConsumerHolder&& other) noexcept = delete;
  ConsumerHolder& operator=(ConfHolder&& other) noexcept = delete;

  rd_kafka_t* Handle() { return handle_.get(); }

 private:
  using HandleHolder = std::unique_ptr<rd_kafka_t, decltype(&rd_kafka_destroy)>;
  HandleHolder handle_;
};

ConsumerImpl::ConsumerImpl(std::unique_ptr<Configuration> configuration)
    : opaque_(configuration->GetComponentName(), EntityType::kConsumer),
      conf_(std::make_unique<ConfHolder>(
          configuration->SetOpaque(opaque_)
              .SetCallbacks([](void* conf) {
                rd_kafka_conf_set_rebalance_cb(
                    static_cast<rd_kafka_conf_t*>(conf), &RebalanceCallback);
                rd_kafka_conf_set_offset_commit_cb(
                    static_cast<rd_kafka_conf_t*>(conf), &OffsetCommitCallback);
              })
              .Release())) {}

ConsumerImpl::~ConsumerImpl() = default;

void ConsumerImpl::Subscribe(const std::vector<std::string>& topics) {
  consumer_ = std::make_unique<ConsumerHolder>(conf_->MakeConfCopy(), err_buf_);

  TopicPartitionsListHolder topic_partitions_list{
      rd_kafka_topic_partition_list_new(topics.size()),
      &rd_kafka_topic_partition_list_destroy};
  for (const auto& topic : topics) {
    rd_kafka_topic_partition_list_add(topic_partitions_list.get(),
                                      topic.c_str(), RD_KAFKA_PARTITION_UA);
  }

  LOG_INFO() << fmt::format("Consumer is subscribing to topics: [{}]",
                            fmt::join(topics, ", "));

  rd_kafka_subscribe(consumer_->Handle(), topic_partitions_list.get());
}

void ConsumerImpl::LeaveGroup() { consumer_.reset(); }

void ConsumerImpl::Resubscribe(const std::vector<std::string>& topics) {
  LeaveGroup();
  LOG_INFO() << "Leaved group";
  Subscribe(topics);
  LOG_INFO() << "Joined group";
}

void ConsumerImpl::Commit() {
  rd_kafka_commit(consumer_->Handle(), nullptr, /*async=*/0);
}

void ConsumerImpl::AsyncCommit() {
  rd_kafka_commit(consumer_->Handle(), nullptr, /*async=*/1);
}

std::optional<Message> ConsumerImpl::PollMessage(engine::Deadline deadline) {
  if (deadline.IsReached()) {
    return std::nullopt;
  }

  const auto poll_timeout{std::chrono::duration_cast<std::chrono::milliseconds>(
      deadline.TimeLeft())};

  LOG_DEBUG() << fmt::format("Polling message for {}ms", poll_timeout.count());

  MessageHolder message{
      rd_kafka_consumer_poll(consumer_->Handle(),
                             static_cast<int>(poll_timeout.count())),
      &rd_kafka_message_destroy};

  if (message == nullptr) {
    return std::nullopt;
  }
  if (message->err != RD_KAFKA_RESP_ERR_NO_ERROR) {
    LOG_WARNING() << fmt::format("Consumed message with error: {}",
                                 rd_kafka_err2str(message->err));
    return std::nullopt;
  }

  Message polled_message{Message::DataStorage{std::move(message)}};

  AccountPolledMessageStat(polled_message);

  LOG_INFO() << fmt::format(
      "Message from kafka topic '{}' received by key '{}' with "
      "partition {} by offset {}",
      polled_message.GetTopic(), polled_message.GetKey(),
      polled_message.GetPartition(), polled_message.GetOffset());

  return polled_message;
}

ConsumerImpl::MessageBatch ConsumerImpl::PollBatch(std::size_t max_batch_size,
                                                   engine::Deadline deadline) {
  MessageBatch batch;

  while (batch.size() < max_batch_size) {
    auto message = PollMessage(deadline);
    if (!message.has_value()) {
      break;
    }
    batch.push_back(std::move(*message));
  }

  if (!batch.empty()) {
    LOG_INFO() << fmt::format("Polled batch of {} messages", batch.size());
  }

  return batch;
}

const Stats& ConsumerImpl::GetStats() const { return opaque_.GetStats(); }

std::shared_ptr<TopicStats> ConsumerImpl::GetTopicStats(
    const std::string& topic) {
  return opaque_.GetStats().topics_stats[topic];
}

void ConsumerImpl::AccountPolledMessageStat(const Message& polled_message) {
  auto topic_stats = GetTopicStats(polled_message.GetTopic());
  ++topic_stats->messages_counts.messages_total;

  const auto message_timestamp = polled_message.GetTimestamp();
  if (message_timestamp) {
    const auto take_time = std::chrono::system_clock::now().time_since_epoch();
    const auto ms_duration =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            take_time - message_timestamp.value())
            .count();
    topic_stats->avg_ms_spent_time.GetCurrentCounter().Account(ms_duration);
  } else {
    LOG_WARNING() << fmt::format(
        "No timestamp in messages to topic '{}' by key '{}'",
        polled_message.GetTopic(), polled_message.GetKey());
  }
}
void ConsumerImpl::AccountMessageProccessingSucceeded(const Message& message) {
  ++GetTopicStats(message.GetTopic())->messages_counts.messages_success;
}

void ConsumerImpl::AccountMessageBatchProccessingSucceeded(
    const MessageBatch& batch) {
  for (const auto& message : batch) {
    AccountMessageProccessingSucceeded(message);
  }
}
void ConsumerImpl::AccountMessageProccessingFailed(const Message& message) {
  ++GetTopicStats(message.GetTopic())->messages_counts.messages_error;
}

void ConsumerImpl::AccountMessageBatchProcessingFailed(
    const MessageBatch& batch) {
  for (const auto& message : batch) {
    AccountMessageProccessingFailed(message);
  }
}

}  // namespace impl

}  // namespace kafka

USERVER_NAMESPACE_END
