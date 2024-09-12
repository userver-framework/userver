#include <kafka/impl/consumer_impl.hpp>

#include <chrono>

#include <fmt/format.h>

#include <userver/logging/log.hpp>
#include <userver/testsuite/testpoint.hpp>
#include <userver/tracing/span.hpp>
#include <userver/utils/fast_scope_guard.hpp>
#include <userver/utils/span.hpp>

#include <kafka/impl/configuration.hpp>
#include <kafka/impl/log_level.hpp>
#include <kafka/impl/stats.hpp>

USERVER_NAMESPACE_BEGIN

namespace kafka {

namespace {

std::optional<std::chrono::milliseconds> RetrieveTimestamp(
    const impl::MessageHolder& message) {
  rd_kafka_timestamp_type_t type = RD_KAFKA_TIMESTAMP_NOT_AVAILABLE;
  const std::int64_t timestamp =
      rd_kafka_message_timestamp(message.GetHandle(), &type);
  if (timestamp == -1 || type == RD_KAFKA_TIMESTAMP_NOT_AVAILABLE) {
    return std::nullopt;
  }

  return std::chrono::milliseconds{timestamp};
}

std::string ToString(rd_kafka_event_type_t event_type) {
  switch (event_type) {
    case RD_KAFKA_EVENT_LOG:
      return "LOG";
    case RD_KAFKA_EVENT_ERROR:
      return "ERROR";
    case RD_KAFKA_EVENT_REBALANCE:
      return "REBALANCE";
    case RD_KAFKA_EVENT_OFFSET_COMMIT:
      return "OFFSET_COMMIT";
    case RD_KAFKA_EVENT_FETCH:
      return "FETCH";
    default:
      return "UNEXPECTED_EVENT";
  }
}

bool IsMessageEvent(const impl::EventHolder& event) {
  return rd_kafka_event_type(event.GetHandle()) == RD_KAFKA_EVENT_FETCH;
}

}  // namespace

struct Message::MessageData final {
  MessageData(impl::MessageHolder message_holder)
      : message(std::move(message_holder)),
        topic(rd_kafka_topic_name(message->rkt)),
        timestamp(RetrieveTimestamp(message)) {}

  MessageData(MessageData&& other) noexcept = default;

  impl::MessageHolder message;

  std::string topic;
  std::optional<std::chrono::milliseconds> timestamp;
};

Message::~Message() = default;

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

Message::Message(impl::MessageHolder&& message) : data_(std::move(message)) {}

namespace impl {

namespace {

void EventCallbackProxy([[maybe_unused]] rd_kafka_t* kafka_client,
                        void* opaque_ptr) {
  UASSERT(kafka_client);
  UASSERT(opaque_ptr);

  static_cast<ConsumerImpl*>(opaque_ptr)->EventCallback();
}

void PrintTopicPartitionsList(
    const rd_kafka_topic_partition_list_t* list,
    std::function<std::string(const rd_kafka_topic_partition_t&)> log,
    bool skip_invalid_offsets = false) {
  if (list == nullptr || list->cnt <= 0) {
    return;
  }

  utils::span<const rd_kafka_topic_partition_t> topic_partitions{
      list->elems, list->elems + static_cast<std::size_t>(list->cnt)};
  for (const auto& topic_partition : topic_partitions) {
    if (skip_invalid_offsets &&
        topic_partition.offset == RD_KAFKA_OFFSET_INVALID) {
      /// @note `librdkafka` does not sets offsets for partitions that were
      /// not committed in the current in the current commit
      LOG_DEBUG() << "Skipping partition " << topic_partition.partition;
      continue;
    }

    LOG_INFO() << log(topic_partition);
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

}  // namespace

void ConsumerImpl::AssignPartitions(
    const rd_kafka_topic_partition_list_t* partitions) {
  LOG_INFO() << "Assigning new partitions to consumer";
  PrintTopicPartitionsList(
      partitions, [](const rd_kafka_topic_partition_t& partition) {
        return fmt::format("Partition {} for topic '{}' assigning",
                           partition.partition, partition.topic);
      });

  const auto assign_err = rd_kafka_assign(consumer_.GetHandle(), partitions);
  if (assign_err != RD_KAFKA_RESP_ERR_NO_ERROR) {
    LOG_ERROR() << fmt::format("Failed to assign partitions: {}",
                               rd_kafka_err2str(assign_err));
    return;
  }

  LOG_INFO() << "Successfully assigned partitions";
}

void ConsumerImpl::RevokePartitions(
    const rd_kafka_topic_partition_list_t* partitions) {
  LOG_INFO() << "Revoking existing partitions from consumer";

  PrintTopicPartitionsList(
      partitions, [](const rd_kafka_topic_partition_t& partition) {
        return fmt::format("Partition {} of '{}' topic revoking",
                           partition.partition, partition.topic);
      });

  const auto revokation_err = rd_kafka_assign(consumer_.GetHandle(), nullptr);
  if (revokation_err != RD_KAFKA_RESP_ERR_NO_ERROR) {
    LOG_ERROR() << fmt::format("Failed to revoke partitions: {}",
                               rd_kafka_err2str(revokation_err));
    return;
  }

  LOG_INFO() << "Successfully revoked partitions";
}

void ConsumerImpl::ErrorCallback(rd_kafka_resp_err_t error, const char* reason,
                                 bool is_fatal) {
  tracing::Span span{"error_callback"};
  span.AddTag("kafka_callback", "error_callback");

  LOG(is_fatal ? logging::Level::kCritical : logging::Level::kError)
      << fmt::format("Error {} occurred because of '{}': {}", error, reason,
                     rd_kafka_err2str(error));

  if (error == RD_KAFKA_RESP_ERR__RESOLVE ||
      error == RD_KAFKA_RESP_ERR__TRANSPORT ||
      error == RD_KAFKA_RESP_ERR__AUTHENTICATION ||
      error == RD_KAFKA_RESP_ERR__ALL_BROKERS_DOWN) {
    ++stats_.connections_error;
  }
}

void ConsumerImpl::LogCallback(const char* facility, const char* message,
                               int log_level) {
  LOG(convertRdKafkaLogLevelToLoggingLevel(log_level))
      << logging::LogExtra{{{"kafka_callback", "log_callback"},
                            {"facility", facility}}}
      << message;
}

void ConsumerImpl::RebalanceCallback(
    rd_kafka_resp_err_t err,
    const rd_kafka_topic_partition_list_t* partitions) {
  tracing::Span span{"rebalance_callback"};
  span.AddTag("kafka_callback", "rebalance_callback");

  LOG_INFO() << fmt::format("Consumer group rebalanced ('{}' protocol)",
                            rd_kafka_rebalance_protocol(consumer_.GetHandle()));

  switch (err) {
    case RD_KAFKA_RESP_ERR__ASSIGN_PARTITIONS:
      AssignPartitions(partitions);
      CallTestpoints(partitions, fmt::format("tp_{}_subscribed", name_));
      break;
    case RD_KAFKA_RESP_ERR__REVOKE_PARTITIONS:
      RevokePartitions(partitions);
      CallTestpoints(partitions, fmt::format("tp_{}_revoked", name_));
      break;
    default:
      LOG_ERROR() << fmt::format("Failed when rebalancing: {}",
                                 rd_kafka_err2str(err));
      break;
  }
}

void ConsumerImpl::OffsetCommitCallback(
    rd_kafka_resp_err_t err,
    const rd_kafka_topic_partition_list_t* committed_offsets) {
  tracing::Span span{"offset_commit_callback"};
  span.AddTag("kafka_callback", "offset_commit_callback");

  if (err != RD_KAFKA_RESP_ERR_NO_ERROR) {
    LOG_ERROR() << fmt::format("Failed to commit offsets: {}",
                               rd_kafka_err2str(err));
    return;
  }

  LOG_INFO() << "Successfully committed offsets";
  PrintTopicPartitionsList(
      committed_offsets,
      [](const rd_kafka_topic_partition_t& offset) {
        return fmt::format(
            "Offset {} committed for topic '{}' within partition "
            "{}",
            offset.offset, offset.topic, offset.partition);
      },
      /*skip_invalid_offsets=*/true);
}

ConsumerImpl::ConsumerImpl(const std::string& name, const ConfHolder& conf,
                           const std::vector<std::string>& topics, Stats& stats)
    : name_(name), stats_(stats), consumer_(conf) {
  StartConsuming(topics);
}

const Stats& ConsumerImpl::GetStats() const { return stats_; }

void ConsumerImpl::StartConsuming(const std::vector<std::string>& topics) {
  rd_kafka_queue_cb_event_enable(consumer_.GetQueue(), &EventCallbackProxy,
                                 this);

  TopicPartitionsListHolder topic_partitions_list{
      rd_kafka_topic_partition_list_new(topics.size())};
  for (const auto& topic : topics) {
    rd_kafka_topic_partition_list_add(topic_partitions_list.GetHandle(),
                                      topic.c_str(), RD_KAFKA_PARTITION_UA);
  }

  LOG_INFO() << fmt::format("Consumer is subscribing to topics: [{}]",
                            fmt::join(topics, ", "));

  rd_kafka_subscribe(consumer_.GetHandle(), topic_partitions_list.GetHandle());
}

void ConsumerImpl::StopConsuming() {
  // disable EventCallback
  rd_kafka_queue_cb_event_enable(consumer_.GetQueue(), nullptr, nullptr);

  utils::FastScopeGuard destroy_consumer_guard{
      [this]() noexcept { consumer_.reset(); }};

  ErrorHolder error{rd_kafka_consumer_close_queue(consumer_.GetHandle(),
                                                  consumer_.GetQueue())};
  if (error) {
    LOG_ERROR() << fmt::format(
        "Failed to properly close consumer: {}",
        rd_kafka_err2str(rd_kafka_error_code(error.GetHandle())));
    return;
  }

  while (!rd_kafka_consumer_closed(consumer_.GetHandle())) {
    if (const EventHolder event = PollEvent()) {
      DispatchEvent(event);
    }
  }
}

void ConsumerImpl::Commit() {
  rd_kafka_commit(consumer_.GetHandle(), nullptr, /*async=*/0);
}

void ConsumerImpl::AsyncCommit() {
  rd_kafka_commit(consumer_.GetHandle(), nullptr, /*async=*/1);
}

EventHolder ConsumerImpl::PollEvent() {
  return EventHolder{
      rd_kafka_queue_poll(consumer_.GetQueue(), /*timeout_ms=*/0)};
}

void ConsumerImpl::DispatchEvent(const EventHolder& event_holder) {
  UASSERT(event_holder);

  auto* event = event_holder.GetHandle();
  switch (rd_kafka_event_type(event)) {
    case RD_KAFKA_EVENT_REBALANCE: {
      RebalanceCallback(rd_kafka_event_error(event),
                        rd_kafka_event_topic_partition_list(event));
    } break;
    case RD_KAFKA_EVENT_OFFSET_COMMIT: {
      OffsetCommitCallback(rd_kafka_event_error(event),
                           rd_kafka_event_topic_partition_list(event));
    } break;
    case RD_KAFKA_EVENT_ERROR:
      ErrorCallback(rd_kafka_event_error(event),
                    rd_kafka_event_error_string(event),
                    rd_kafka_event_error_is_fatal(event));
      break;
    case RD_KAFKA_EVENT_LOG: {
      const char* facility{nullptr};
      const char* message{nullptr};
      int log_level{};
      rd_kafka_event_log(event, &facility, &message, &log_level);
      LogCallback(facility, message, log_level);
    } break;
  }
}

void ConsumerImpl::EventCallback() {
  /// The callback is called from internal librdkafka thread, i.e. not in
  /// coroutine environment, therefore not all synchronization
  /// primitives can be used in the callback body.

  LOG_INFO()
      << "Consumer events queue became non-empty. Waking up message poller";
  queue_became_non_empty_event_.Send();
}

std::optional<Message> ConsumerImpl::TakeEventMessage(
    EventHolder&& event_holder) {
  UASSERT(IsMessageEvent(event_holder));
  UASSERT(rd_kafka_event_message_count(event_holder.GetHandle()) == 1);

  MessageHolder message{std::move(event_holder)};
  if (message->err != RD_KAFKA_RESP_ERR_NO_ERROR) {
    LOG_WARNING() << fmt::format("Polled messages contains an error: {}",
                                 rd_kafka_err2str(message->err));

    return std::nullopt;
  }
  Message polled_message{std::move(message)};

  AccountPolledMessageStat(polled_message);

  LOG_DEBUG() << fmt::format(
      "Message from kafka topic '{}' received by key '{}' with "
      "partition {} by offset {}",
      polled_message.GetTopic(), polled_message.GetKey(),
      polled_message.GetPartition(), polled_message.GetOffset());

  return polled_message;
}

std::optional<Message> ConsumerImpl::PollMessage(engine::Deadline deadline) {
  bool just_waked_up{false};

  while (!deadline.IsReached() || std::exchange(just_waked_up, false)) {
    const auto time_left_ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            deadline.TimeLeft())
            .count();
    LOG_DEBUG() << fmt::format("Polling message for {}ms", time_left_ms);

    if (EventHolder event = PollEvent()) {
      LOG_DEBUG() << fmt::format(
          "Polled {} event", ToString(rd_kafka_event_type(event.GetHandle())));

      if (IsMessageEvent(event)) {
        return TakeEventMessage(std::move(event));
      } else {
        DispatchEvent(event);
      }
    } else {
      LOG_DEBUG() << fmt::format(
          "No sufficient messages are available, suspending consumer execution "
          "for at most {}ms",
          time_left_ms);

      if (!queue_became_non_empty_event_.WaitForEventUntil(deadline)) {
        LOG_DEBUG() << fmt::format(
            "No messages still available after {}ms (or polling task was "
            "canceled)",
            time_left_ms);

        return std::nullopt;
      }
      LOG_DEBUG() << "New events are available, poll them immediately";
      just_waked_up = true;
    }
  }

  return std::nullopt;
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

std::shared_ptr<TopicStats> ConsumerImpl::GetTopicStats(
    const std::string& topic) {
  return stats_.topics_stats[topic];
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
void ConsumerImpl::AccountMessageProcessingSucceeded(const Message& message) {
  ++GetTopicStats(message.GetTopic())->messages_counts.messages_success;
}

void ConsumerImpl::AccountMessageBatchProcessingSucceeded(
    const MessageBatch& batch) {
  for (const auto& message : batch) {
    AccountMessageProcessingSucceeded(message);
  }
}
void ConsumerImpl::AccountMessageProcessingFailed(const Message& message) {
  ++GetTopicStats(message.GetTopic())->messages_counts.messages_error;
}

void ConsumerImpl::AccountMessageBatchProcessingFailed(
    const MessageBatch& batch) {
  for (const auto& message : batch) {
    AccountMessageProcessingFailed(message);
  }
}

}  // namespace impl

}  // namespace kafka

USERVER_NAMESPACE_END
