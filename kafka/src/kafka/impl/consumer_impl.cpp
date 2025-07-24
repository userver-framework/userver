#include <kafka/impl/consumer_impl.hpp>

#include <algorithm>
#include <chrono>
#include <limits>

#include <fmt/ranges.h>

#include <userver/kafka/exceptions.hpp>
#include <userver/kafka/impl/configuration.hpp>
#include <userver/kafka/impl/stats.hpp>
#include <userver/logging/log.hpp>
#include <userver/testsuite/testpoint.hpp>
#include <userver/tracing/span.hpp>
#include <userver/utils/encoding/hex.hpp>
#include <userver/utils/fast_scope_guard.hpp>
#include <userver/utils/span.hpp>

#include <kafka/impl/error_buffer.hpp>
#include <kafka/impl/holders_aliases.hpp>
#include <kafka/impl/log_level.hpp>

USERVER_NAMESPACE_BEGIN

namespace kafka {

namespace {

std::optional<std::chrono::milliseconds> RetrieveTimestamp(const impl::MessageHolder& message) {
    rd_kafka_timestamp_type_t type = RD_KAFKA_TIMESTAMP_NOT_AVAILABLE;
    const std::int64_t timestamp = rd_kafka_message_timestamp(message.GetHandle(), &type);
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

const rd_kafka_headers_t* ParseHeaders(const rd_kafka_message_t* message) {
    rd_kafka_headers_t* headers_ptr{nullptr};
    switch (auto error = rd_kafka_message_headers(message, &headers_ptr)) {
        case RD_KAFKA_RESP_ERR_NO_ERROR:
            return headers_ptr;
        case RD_KAFKA_RESP_ERR__NOENT:
            return nullptr;
        default:
            throw ParseHeadersException{rd_kafka_err2str(error)};
    }
}

int ToRdKafkaTimeout(engine::Deadline deadline) {
    return static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(deadline.TimeLeft()).count());
}

}  // namespace

struct Message::MessageData final {
    explicit MessageData(impl::MessageHolder message_holder)
        : message(std::move(message_holder)),
          topic(rd_kafka_topic_name(message->rkt)),
          timestamp(RetrieveTimestamp(message)) {}

    MessageData(MessageData&& other) noexcept = default;

    impl::MessageHolder message;

    std::string topic;
    std::optional<std::chrono::milliseconds> timestamp;
};

Message::Message(impl::MessageHolder&& message) : data_(std::move(message)) {}

Message::Message(Message&&) noexcept = default;

Message::~Message() = default;

const std::string& Message::GetTopic() const { return data_->topic; }

std::string_view Message::GetKey() const {
    const auto* key = data_->message->key;

    if (!key) {
        return std::string_view{};
    }

    return std::string_view{static_cast<const char*>(key), data_->message->key_len};
}

std::string_view Message::GetPayload() const {
    return std::string_view{static_cast<const char*>(data_->message->payload), data_->message->len};
}

std::optional<std::chrono::milliseconds> Message::GetTimestamp() const { return data_->timestamp; }

int Message::GetPartition() const { return data_->message->partition; }

std::int64_t Message::GetOffset() const { return data_->message->offset; }

HeadersReader Message::GetHeaders() const& { return HeadersReader{ParseHeaders(data_->message.GetHandle())}; }

std::optional<std::string_view> Message::GetHeader(utils::zstring_view name) const {
    const auto* headers = ParseHeaders(data_->message.GetHandle());
    if (headers == nullptr) {
        return std::nullopt;
    }

    const void* value{};
    std::size_t value_size{};
    switch (rd_kafka_header_get_last(headers, name.c_str(), &value, &value_size)) {
        case RD_KAFKA_RESP_ERR_NO_ERROR:
            return std::string_view{static_cast<const char*>(value), value_size};
        default:
            return std::nullopt;
    }
}

namespace impl {

namespace {

enum class RebalanceProtocol {
    kCooperative,
    kEager,
};

RebalanceProtocol GetProtocol(const ConsumerHolder& consumer) {
    constexpr utils::zstring_view kCooperative{"COOPERATIVE"};
    if (rd_kafka_rebalance_protocol(consumer.GetHandle()) == kCooperative) {
        return RebalanceProtocol::kCooperative;
    }
    return RebalanceProtocol::kEager;
}

void EventCallbackProxy([[maybe_unused]] rd_kafka_t* kafka_client, void* opaque_ptr) {
    UASSERT(kafka_client);
    UASSERT(opaque_ptr);

    static_cast<ConsumerImpl*>(opaque_ptr)->EventCallback();
}

void PrintTopicPartitionsList(
    const rd_kafka_topic_partition_list_t* list,
    std::function<std::string(const rd_kafka_topic_partition_t&)> log,
    const logging::Level log_level = logging::Level::kInfo,
    bool skip_invalid_offsets = false
) {
    if (list == nullptr || list->cnt <= 0) {
        return;
    }

    const utils::span<const rd_kafka_topic_partition_t> topic_partitions{
        list->elems, list->elems + static_cast<std::size_t>(list->cnt)};
    for (const auto& topic_partition : topic_partitions) {
        if (skip_invalid_offsets && topic_partition.offset == RD_KAFKA_OFFSET_INVALID) {
            /// @note `librdkafka` does not sets offsets for partitions that were
            /// not committed in the current in the current commit
            LOG_DEBUG("Skipping partition {}", topic_partition.partition);
            continue;
        }

        LOG(log_level) << log(topic_partition);
    }
}

void CallTestpoints(const rd_kafka_topic_partition_list_t* list, const std::string& testpoint_name) {
    if (list == nullptr || list->cnt == 0 || !testsuite::AreTestpointsAvailable()) {
        return;
    }

    for (int i = 0; i < list->cnt; ++i) {
        TESTPOINT(testpoint_name, {});
    }
}

std::string GetMessageKeyForLogging(MessageKeyLogFormat log_format, const Message& message) {
    switch (log_format) {
        case MessageKeyLogFormat::kHex:
            return utils::encoding::ToHex(message.GetKey());
        case MessageKeyLogFormat::kPlainText:
            return std::string(message.GetKey());
        default:
            UINVARIANT(false, "Unsupported message key log format");
            return {};
    }
}

}  // namespace

bool ConsumerImpl::AssignPartitions(const rd_kafka_topic_partition_list_t* partitions) {
    const auto rebalance_protocol = GetProtocol(consumer_);

    LOG(execution_params_.debug_info_log_level,
        "Assigning new partitions to consumer ('{}' protocol)",
        rd_kafka_rebalance_protocol(consumer_.GetHandle()));
    PrintTopicPartitionsList(
        partitions,
        [](const rd_kafka_topic_partition_t& partition) {
            return fmt::format("Partition {} for topic '{}' assigning", partition.partition, partition.topic);
        },
        /*log_level=*/execution_params_.operation_log_level
    );

    switch (rebalance_protocol) {
        case RebalanceProtocol::kCooperative: {
            const ErrorHolder assign_err{rd_kafka_incremental_assign(consumer_.GetHandle(), partitions)};
            if (assign_err) {
                LOG_ERROR(
                    "Failed to incrementally assign partitions: {}", rd_kafka_error_string(assign_err.GetHandle())
                );
                return false;
            }
        } break;
        case RebalanceProtocol::kEager: {
            const auto assign_err = rd_kafka_assign(consumer_.GetHandle(), partitions);
            if (assign_err != RD_KAFKA_RESP_ERR_NO_ERROR) {
                LOG_ERROR("Failed to assign partitions: {}", rd_kafka_err2str(assign_err));
                return false;
            }
        } break;
    }

    LOG(execution_params_.debug_info_log_level) << "Successfully assigned partitions";
    return true;
}

bool ConsumerImpl::RevokePartitions(const rd_kafka_topic_partition_list_t* partitions) {
    const auto rebalance_protocol = GetProtocol(consumer_);

    LOG(execution_params_.debug_info_log_level,
        "Revoking existing partitions from consumer ('{}' protocol)",
        rd_kafka_rebalance_protocol(consumer_.GetHandle()));

    PrintTopicPartitionsList(
        partitions,
        [](const rd_kafka_topic_partition_t& partition) {
            return fmt::format("Partition {} of '{}' topic revoking", partition.partition, partition.topic);
        },
        /*log_level=*/execution_params_.operation_log_level
    );

    switch (rebalance_protocol) {
        case RebalanceProtocol::kCooperative: {
            const ErrorHolder revocation_err{rd_kafka_incremental_unassign(consumer_.GetHandle(), partitions)};
            if (revocation_err) {
                LOG_ERROR(
                    "Failed to incrementally revoke partitions: {}", rd_kafka_error_string(revocation_err.GetHandle())
                );
                return false;
            }
        } break;
        case RebalanceProtocol::kEager: {
            const auto revocation_err = rd_kafka_assign(consumer_.GetHandle(), nullptr);
            if (revocation_err != RD_KAFKA_RESP_ERR_NO_ERROR) {
                LOG_ERROR("Failed to revoke partitions: {}", rd_kafka_err2str(revocation_err));
                return false;
            }
        } break;
    }

    LOG(execution_params_.debug_info_log_level) << "Successfully revoked partitions";
    return true;
}

void ConsumerImpl::ErrorCallback(rd_kafka_resp_err_t error, const char* reason, bool is_fatal) {
    tracing::Span span{"error_callback"};
    span.AddTag("kafka_callback", "error_callback");

    LOG(is_fatal ? logging::Level::kCritical : logging::Level::kError
    ) << fmt::format("Error {} occurred because of '{}': {}", static_cast<int>(error), reason, rd_kafka_err2str(error));

    if (error == RD_KAFKA_RESP_ERR__RESOLVE || error == RD_KAFKA_RESP_ERR__TRANSPORT ||
        error == RD_KAFKA_RESP_ERR__AUTHENTICATION || error == RD_KAFKA_RESP_ERR__ALL_BROKERS_DOWN) {
        ++stats_.connections_error;
    }
}

void ConsumerImpl::LogCallback(const char* facility, const char* message, int log_level) {
    LOG(convertRdKafkaLogLevelToLoggingLevel(log_level))
        << logging::LogExtra{{{"kafka_callback", "log_callback"}, {"facility", facility}}} << message;
}

void ConsumerImpl::RebalanceCallback(rd_kafka_resp_err_t err, const rd_kafka_topic_partition_list_t* partitions) {
    tracing::Span span{"rebalance_callback"};
    span.AddTag("kafka_callback", "rebalance_callback");

    LOG(execution_params_.operation_log_level,
        "Consumer group rebalanced ('{}' protocol)",
        rd_kafka_rebalance_protocol(consumer_.GetHandle()));

    switch (err) {
        case RD_KAFKA_RESP_ERR__ASSIGN_PARTITIONS: {
            const bool ok = AssignPartitions(partitions);
            CallTestpoints(partitions, fmt::format("tp_{}_subscribed", name_));
            if (ok) {
                CallUserRebalanceCallback(partitions, RebalanceEventType::kAssigned);
            }
            break;
        }
        case RD_KAFKA_RESP_ERR__REVOKE_PARTITIONS: {
            const bool ok = RevokePartitions(partitions);
            CallTestpoints(partitions, fmt::format("tp_{}_revoked", name_));
            if (ok) {
                CallUserRebalanceCallback(partitions, RebalanceEventType::kRevoked);
            }
            break;
        }
        default:
            LOG_ERROR("Failed when rebalancing: {}", rd_kafka_err2str(err));
            break;
    }
}

void ConsumerImpl::CallUserRebalanceCallback(
    const rd_kafka_topic_partition_list_s* partitions,
    RebalanceEventType event_type
) noexcept try {
    tracing::Span span{"kafka_user_rebalance_callback"};

    if (!rebalance_callback_ || partitions == nullptr || partitions->cnt <= 0) {
        return;
    }

    utils::span<const rd_kafka_topic_partition_t> kafka_topic_partitions{
        partitions->elems, partitions->elems + static_cast<std::size_t>(partitions->cnt)};
    std::vector<TopicPartitionView> topic_partitions;
    topic_partitions.reserve(kafka_topic_partitions.size());

    for (const auto& topic_partition : kafka_topic_partitions) {
        if (topic_partition.partition < 0) {
            LOG_ERROR(
                "Skipped topic: {} partition: {} for user's rebalance callback, because got "
                "negative number for partition id from librdkafka.",
                topic_partition.topic,
                topic_partition.partition
            );
            continue;
        }

        std::optional<std::uint64_t> partition_offset;
        if (topic_partition.offset > 0) {
            partition_offset = static_cast<std::uint64_t>(topic_partition.offset);
        }

        topic_partitions.emplace_back(
            topic_partition.topic, static_cast<std::uint32_t>(topic_partition.partition), partition_offset
        );
    }

    if (!topic_partitions.empty()) {
        (*rebalance_callback_)(topic_partitions, event_type);
    }
} catch (const std::exception& exc) {
    LOG_ERROR("User's rebalance callback thrown an exception: {}", exc.what());
} catch (...) {
    LOG_ERROR("User's rebalance callback thrown unknown exception.");
}

void ConsumerImpl::OffsetCommitCallback(
    rd_kafka_resp_err_t err,
    const rd_kafka_topic_partition_list_t* committed_offsets
) {
    tracing::Span span{"offset_commit_callback"};
    span.AddTag("kafka_callback", "offset_commit_callback");

    if (err != RD_KAFKA_RESP_ERR_NO_ERROR) {
        LOG_ERROR("Failed to commit offsets: {}", rd_kafka_err2str(err));
        return;
    }

    LOG(execution_params_.debug_info_log_level) << "Successfully committed offsets";
    PrintTopicPartitionsList(
        committed_offsets,
        [](const rd_kafka_topic_partition_t& offset) {
            return fmt::format(
                "Offset {} committed for topic '{}' within partition "
                "{}",
                offset.offset,
                offset.topic,
                offset.partition
            );
        },
        /*log_level=*/execution_params_.debug_info_log_level,
        /*skip_invalid_offsets=*/true
    );
}

ConsumerImpl::ConsumerImpl(
    const std::string& name,
    const ConfHolder& conf,
    const std::vector<std::string>& topics,
    const ConsumerExecutionParams& execution_params,
    const std::optional<ConsumerRebalanceCallback>& rebalance_callback_opt,
    Stats& stats
)
    : name_(name),
      topics_(topics),
      execution_params_(execution_params),
      rebalance_callback_(rebalance_callback_opt),
      stats_(stats),
      consumer_(conf) {}

const Stats& ConsumerImpl::GetStats() const { return stats_; }

void ConsumerImpl::StartConsuming() {
    rd_kafka_queue_cb_event_enable(consumer_.GetQueue(), &EventCallbackProxy, this);

    const TopicPartitionsListHolder topic_partitions_list{rd_kafka_topic_partition_list_new(topics_.size())};
    for (const auto& topic : topics_) {
        rd_kafka_topic_partition_list_add(topic_partitions_list.GetHandle(), topic.c_str(), RD_KAFKA_PARTITION_UA);
    }

    LOG(execution_params_.operation_log_level)
        << fmt::format("Consumer is subscribing to topics: [{}]", fmt::join(topics_, ", "));
    // Only initiates subscribe process
    auto err = rd_kafka_subscribe(consumer_.GetHandle(), topic_partitions_list.GetHandle());
    if (err != RD_KAFKA_RESP_ERR_NO_ERROR) {
        throw std::runtime_error{fmt::format("Consumer failed to subscribe: {}", rd_kafka_err2str(err))};
    }
}

void ConsumerImpl::StopConsuming() {
    // disable EventCallback
    rd_kafka_queue_cb_event_enable(consumer_.GetQueue(), nullptr, nullptr);

    // launch closing process
    const ErrorHolder error{rd_kafka_consumer_close_queue(consumer_.GetHandle(), consumer_.GetQueue())};
    if (error) {
        LOG_ERROR("Failed to properly close consumer: {}", rd_kafka_err2str(rd_kafka_error_code(error.GetHandle())));
        return;
    }

    // poll until queue is closed
    while (!rd_kafka_consumer_closed(consumer_.GetHandle())) {
        if (const EventHolder event = PollEvent()) {
            DispatchEvent(event);
        }
    }
}

void ConsumerImpl::Commit() { rd_kafka_commit(consumer_.GetHandle(), nullptr, /*async=*/0); }

void ConsumerImpl::AsyncCommit() { rd_kafka_commit(consumer_.GetHandle(), nullptr, /*async=*/1); }

OffsetRange ConsumerImpl::GetOffsetRange(
    utils::zstring_view topic,
    std::uint32_t partition,
    std::optional<std::chrono::milliseconds> timeout
) const {
    std::int64_t low_offset{0};
    std::int64_t high_offset{0};

    auto err = rd_kafka_query_watermark_offsets(
        consumer_.GetHandle(),
        topic.c_str(),
        partition,
        &low_offset,
        &high_offset,
        static_cast<int>(timeout.value_or(std::chrono::milliseconds(-1)).count())
    );

    if (err == RD_KAFKA_RESP_ERR__TIMED_OUT) {
        throw OffsetRangeTimeoutException{topic, partition};
    }
    if (err != RD_KAFKA_RESP_ERR_NO_ERROR) {
        throw OffsetRangeException{fmt::format("Failed to get offsets: {}", rd_kafka_err2str(err)), topic, partition};
    }
    if (low_offset == RD_KAFKA_OFFSET_INVALID || high_offset == RD_KAFKA_OFFSET_INVALID || low_offset < 0 ||
        high_offset < 0) {
        throw OffsetRangeException{"Failed to get offsets: invalid offset.", topic, partition};
    }

    return {static_cast<std::uint64_t>(low_offset), static_cast<std::uint64_t>(high_offset)};
}

std::vector<std::uint32_t>
ConsumerImpl::GetPartitionIds(utils::zstring_view topic, std::optional<std::chrono::milliseconds> timeout) const {
    const MetadataHolder metadata{[this, &topic, &timeout] {
        const rd_kafka_metadata_t* raw_metadata{nullptr};
        const TopicHolder topic_holder{rd_kafka_topic_new(consumer_.GetHandle(), topic.c_str(), nullptr)};
        if (!topic_holder) {
            throw GetMetadataException{fmt::format("Failed to create new rdkafka topic with name: {}", topic)};
        }

        auto err = rd_kafka_metadata(
            consumer_.GetHandle(),
            /*all_topics=*/0,
            topic_holder.GetHandle(),
            &raw_metadata,
            static_cast<int>(timeout.value_or(std::chrono::milliseconds(-1)).count())
        );
        if (err == RD_KAFKA_RESP_ERR__TIMED_OUT) {
            throw GetMetadataTimeoutException{topic};
        }
        if (err != RD_KAFKA_RESP_ERR_NO_ERROR) {
            throw GetMetadataException{fmt::format("Failed to fetch metadata: {}.", rd_kafka_err2str(err)), topic};
        }
        return raw_metadata;
    }()};

    const utils::span<const rd_kafka_metadata_topic> topics{
        metadata->topics, static_cast<std::size_t>(metadata->topic_cnt)};
    const auto* topic_it =
        std::find_if(topics.begin(), topics.end(), [&topic](const rd_kafka_metadata_topic& topic_raw) {
            return topic == topic_raw.topic;
        });
    if (topic_it == topics.end()) {
        throw TopicNotFoundException{fmt::format("Failed to find topic: {}", topic)};
    }

    const utils::span<const rd_kafka_metadata_partition> partitions{
        topic_it->partitions, static_cast<std::size_t>(topic_it->partition_cnt)};
    std::vector<std::uint32_t> partition_ids;
    partition_ids.reserve(partitions.size());

    for (const auto& partition : partitions) {
        partition_ids.push_back(partition.id);
    }

    return partition_ids;
}

EventHolder ConsumerImpl::PollEvent() {
    return EventHolder{rd_kafka_queue_poll(consumer_.GetQueue(), /*timeout_ms=*/0)};
}

void ConsumerImpl::DispatchEvent(const EventHolder& event_holder) {
    UASSERT(event_holder);

    auto* event = event_holder.GetHandle();
    switch (rd_kafka_event_type(event)) {
        case RD_KAFKA_EVENT_REBALANCE: {
            RebalanceCallback(rd_kafka_event_error(event), rd_kafka_event_topic_partition_list(event));
        } break;
        case RD_KAFKA_EVENT_OFFSET_COMMIT: {
            OffsetCommitCallback(rd_kafka_event_error(event), rd_kafka_event_topic_partition_list(event));
        } break;
        case RD_KAFKA_EVENT_ERROR:
            ErrorCallback(
                rd_kafka_event_error(event), rd_kafka_event_error_string(event), rd_kafka_event_error_is_fatal(event)
            );
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

    LOG(execution_params_.debug_info_log_level) << "Consumer events queue became non-empty. Waking up message poller";
    queue_became_non_empty_event_.Send();
}

std::optional<Message> ConsumerImpl::TakeEventMessage(EventHolder&& event_holder) {
    UASSERT(IsMessageEvent(event_holder));
    UASSERT(rd_kafka_event_message_count(event_holder.GetHandle()) == 1);

    MessageHolder message{event_holder.release()};
    if (message->err != RD_KAFKA_RESP_ERR_NO_ERROR) {
        LOG_WARNING("Polled messages contains an error: {}", rd_kafka_err2str(message->err));

        return std::nullopt;
    }
    Message polled_message{std::move(message)};

    AccountPolledMessageStat(polled_message);

    LOG(execution_params_.operation_log_level) << fmt::format(
        "Message from kafka topic '{}' received by key '{}' with "
        "partition {} by offset {}",
        polled_message.GetTopic(),
        GetMessageKeyForLogging(execution_params_.message_key_log_format, polled_message),
        polled_message.GetPartition(),
        polled_message.GetOffset()
    );

    return polled_message;
}

std::optional<Message> ConsumerImpl::PollMessage(engine::Deadline deadline) {
    bool just_waked_up{false};

    while (!deadline.IsReached() || std::exchange(just_waked_up, false)) {
        const auto time_left_ms = ToRdKafkaTimeout(deadline);
        LOG(execution_params_.debug_info_log_level) << fmt::format("Polling message for {}ms", time_left_ms);
        if (EventHolder event = PollEvent()) {
            LOG(execution_params_.debug_info_log_level)
                << fmt::format("Polled {} event", ToString(rd_kafka_event_type(event.GetHandle())));

            if (IsMessageEvent(event)) {
                return TakeEventMessage(std::move(event));
            } else {
                DispatchEvent(event);
            }
        } else {
            LOG(execution_params_.debug_info_log_level) << fmt::format(
                "No sufficient messages are available, suspending consumer execution for at most {}ms", time_left_ms
            );

            if (!queue_became_non_empty_event_.WaitForEventUntil(deadline)) {
                LOG(execution_params_.debug_info_log_level
                ) << fmt::format("No messages still available after {}ms (or polling task was canceled)", time_left_ms);
                return std::nullopt;
            }
            LOG(execution_params_.debug_info_log_level) << "New events are available, poll them immediately";
            just_waked_up = true;
        }
    }

    return std::nullopt;
}

ConsumerImpl::MessageBatch ConsumerImpl::PollBatch(std::size_t max_batch_size, engine::Deadline deadline) {
    MessageBatch batch;
    while (batch.size() < max_batch_size) {
        auto message = PollMessage(deadline);
        if (!message.has_value()) {
            break;
        }
        batch.push_back(std::move(*message));
    }

    if (!batch.empty()) {
        LOG(execution_params_.debug_info_log_level) << fmt::format("Polled batch of {} messages", batch.size());
    }

    return batch;
}

std::shared_ptr<TopicStats> ConsumerImpl::GetTopicStats(const std::string& topic) { return stats_.topics_stats[topic]; }

void ConsumerImpl::AccountPolledMessageStat(const Message& polled_message) {
    auto topic_stats = GetTopicStats(polled_message.GetTopic());
    ++topic_stats->messages_counts.messages_total;

    const auto message_timestamp = polled_message.GetTimestamp();
    if (message_timestamp) {
        const auto take_time = std::chrono::system_clock::now().time_since_epoch();
        const auto ms_duration =
            std::chrono::duration_cast<std::chrono::milliseconds>(take_time - message_timestamp.value()).count();
        topic_stats->avg_ms_spent_time.GetCurrentCounter().Account(ms_duration);
    } else {
        LOG_WARNING(
            "No timestamp in messages to topic '{}' by key '{}'",
            polled_message.GetTopic(),
            GetMessageKeyForLogging(execution_params_.message_key_log_format, polled_message)
        );
    }
}
void ConsumerImpl::AccountMessageProcessingSucceeded(const Message& message) {
    ++GetTopicStats(message.GetTopic())->messages_counts.messages_success;
}

void ConsumerImpl::AccountMessageBatchProcessingSucceeded(const MessageBatch& batch) {
    for (const auto& message : batch) {
        AccountMessageProcessingSucceeded(message);
    }
}
void ConsumerImpl::AccountMessageProcessingFailed(const Message& message) {
    ++GetTopicStats(message.GetTopic())->messages_counts.messages_error;
}

void ConsumerImpl::AccountMessageBatchProcessingFailed(const MessageBatch& batch) {
    for (const auto& message : batch) {
        AccountMessageProcessingFailed(message);
    }
}

void ConsumerImpl::Seek(
    utils::zstring_view topic,
    std::uint32_t partition_id,
    std::uint64_t offset,
    std::chrono::milliseconds timeout
) const {
    if (offset > std::numeric_limits<std::int64_t>::max()) {
        throw SeekInvalidArgumentException(
            fmt::format("Offset value have to be <= std::int64_t::max() . offset: {}", offset)
        );
    }

    SeekToOffset(topic, partition_id, static_cast<std::int64_t>(offset), timeout);
}

void ConsumerImpl::SeekToOffset(
    utils::zstring_view topic,
    std::uint32_t partition_id,
    std::int64_t offset,
    std::chrono::milliseconds timeout
) const {
    if (timeout.count() <= 0) {
        throw SeekInvalidArgumentException(fmt::format("Timeout value have to be > 0. value: {}ms", timeout.count()));
    }

    const auto deadline = engine::Deadline::FromDuration(timeout);

    // Here, `rd_kafka_queue_poll` call is required to activate the assign operation
    // Namely, after `rd_kafka_assign` called, poll must be called to process the operation
    // and assignment take effect
    { EventHolder event{rd_kafka_queue_poll(consumer_.GetQueue(), ToRdKafkaTimeout(deadline))}; }

    TopicPartitionsListHolder topic_partitions_list{rd_kafka_topic_partition_list_new(1)};
    rd_kafka_topic_partition_t* part = rd_kafka_topic_partition_list_add(
        topic_partitions_list.GetHandle(), topic.c_str(), static_cast<std::int32_t>(partition_id)
    );
    part->offset = offset;

    PrintTopicPartitionsList(topic_partitions_list.GetHandle(), [](const rd_kafka_topic_partition_t& partition) {
        return fmt::format(
            "Partition {} for topic '{}' seeking to offset: {}", partition.partition, partition.topic, partition.offset
        );
    });

    const auto* err =
        rd_kafka_seek_partitions(consumer_.GetHandle(), topic_partitions_list.GetHandle(), ToRdKafkaTimeout(deadline));
    if (err == nullptr) {
        LOG_INFO(
            "Seeked to offset: {}"
            " for partition: {} topic: {} successfully",
            offset,
            partition_id,
            topic
        );
        return;
    }

    throw SeekException(fmt::format(
        "Failed to seek topic: {} partition_id: {} to a given offset. err: {}",
        topic,
        partition_id,
        rd_kafka_error_string(err)
    ));
}

void ConsumerImpl::SeekToEnd(utils::zstring_view topic, std::uint32_t partition_id, std::chrono::milliseconds timeout)
    const {
    SeekToOffset(topic, partition_id, RD_KAFKA_OFFSET_END, timeout);
}

void ConsumerImpl::SeekToBeginning(
    utils::zstring_view topic,
    std::uint32_t partition_id,
    std::chrono::milliseconds timeout
) const {
    SeekToOffset(topic, partition_id, RD_KAFKA_OFFSET_BEGINNING, timeout);
}

}  // namespace impl

}  // namespace kafka

USERVER_NAMESPACE_END
