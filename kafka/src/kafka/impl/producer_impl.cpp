#include <kafka/impl/producer_impl.hpp>

#include <userver/logging/log_extra.hpp>

#include <kafka/impl/delivery_waiter.hpp>
#include <kafka/impl/error_buffer.hpp>

#include <librdkafka/rdkafka.h>

#include <utility>

USERVER_NAMESPACE_BEGIN

namespace kafka {

namespace impl {

class ProducerHolder {
 public:
  ProducerHolder(rd_kafka_t* producer) : handle_(producer) {}

  ~ProducerHolder() {
    UASSERT_MSG(handle_ == nullptr,
                "ProducerHolder must be released before dctor called");
  }

  rd_kafka_t* Handle() { return handle_; }
  rd_kafka_t* Release() { return std::exchange(handle_, nullptr); }

 private:
  rd_kafka_t* handle_;
};

}  // namespace impl

namespace {

rd_kafka_t* MakeProducerFromConf(rd_kafka_conf_t* conf) {
  impl::ErrorBuffer err_buf{};

  rd_kafka_t* producer =
      rd_kafka_new(RD_KAFKA_PRODUCER, conf, err_buf.data(), err_buf.size());
  if (producer == nullptr) {
    /// @note `librdkafka` takes ownership on conf iff `rd_kafka_new`
    /// succeeds
    rd_kafka_conf_destroy(conf);

    impl::PrintErrorAndThrow("create producer", err_buf);
  }

  return producer;
}

/// @note Message delivery may timeout due to timeout fired when request to
/// broker was in-flight. That case, message will have status
/// `RD_KAFKA_MSG_STATUS_POSSIBLY_PERSISTED`. It is not safe to retry such
/// messages, because of reordering and duplication possibility
/// @see
/// https://docs.confluent.io/platform/current/clients/librdkafka/html/md_INTRODUCTION.html#autotoc_md21
bool IsRetryable(rd_kafka_resp_err_t err,
                 std::optional<rd_kafka_msg_status_t> status = std::nullopt) {
  return (err == RD_KAFKA_RESP_ERR__MSG_TIMED_OUT ||
          err == RD_KAFKA_RESP_ERR__QUEUE_FULL ||
          err == RD_KAFKA_RESP_ERR_INVALID_REPLICATION_FACTOR) &&
         status != RD_KAFKA_MSG_STATUS_PERSISTED &&
         status != RD_KAFKA_MSG_STATUS_POSSIBLY_PERSISTED;
}

}  // namespace

ProducerImpl::ProducerImpl(
    void* conf, std::unique_ptr<impl::StatsAndLogging> stats_and_logging)
    : producer_(std::make_unique<impl::ProducerHolder>(
          MakeProducerFromConf(static_cast<rd_kafka_conf_t*>(conf)))),
      stats_and_logging_(std::move(stats_and_logging)) {}

ProducerImpl::~ProducerImpl() {
  rd_kafka_t* producer = producer_->Release();

  if (rd_kafka_flush(producer, kCoolDownFlushTimeout.count()) ==
      RD_KAFKA_RESP_ERR__TIMED_OUT) {
    LOG_WARNING() << GetLoggingTags()
                  << "Producer flushing timeouted on producer destroy. Some "
                     "messages may be not delivered!!!";
  }
  rd_kafka_destroy(producer);
}

const impl::Stats& ProducerImpl::GetStats() const {
  return *stats_and_logging_->stats;
}

const logging::LogExtra& ProducerImpl::GetLoggingTags() const {
  return stats_and_logging_->log_tags_;
}

void ProducerImpl::Send(const std::string& topic_name, std::string_view key,
                        std::string_view message,
                        std::optional<std::uint32_t> partition,
                        std::size_t retries) const {
  using enum SendResult;

  auto& topic_stats = stats_and_logging_->stats->topics_stats[topic_name];
  ++topic_stats->messages_counts.messages_total;

  const auto produce_start_time =
      std::chrono::system_clock::now().time_since_epoch();

  std::size_t retries_left{retries};
  const auto ShouldRetry = [this, &retries_left](SendResult send_result) {
    if (retries_left > 0 && send_result == Retryable) {
      LOG_WARNING() << GetLoggingTags()
                    << "Send request failed, but error may be transient, "
                       "retrying..."
                    << fmt::format("(retries left: {})", retries_left);
      retries_left -= 1;
      return true;
    }

    return false;
  };

  std::optional<SendResult> send_result;
  do {
    send_result = SendImpl(topic_name, key, message, partition);
  } while (ShouldRetry(send_result.value()));

  const auto produce_finish_time =
      std::chrono::system_clock::now().time_since_epoch();
  const auto produce_duration_ms =
      std::chrono::duration_cast<std::chrono::milliseconds>(
          produce_finish_time - produce_start_time)
          .count();
  topic_stats->avg_ms_spent_time.GetCurrentCounter().Account(
      produce_duration_ms);

  LOG_INFO() << GetLoggingTags()
             << fmt::format("Message produced to topic '{}' in {}ms {}",
                            topic_name, produce_duration_ms,
                            retries > retries_left
                                ? fmt::format("(after {} retries)",
                                              retries - retries_left)
                                : "");

  switch (send_result.value()) {
    case SendResult::Succeeded: {
      ++topic_stats->messages_counts.messages_success;
      break;
    }
    case SendResult::Failed:
    case SendResult::Retryable: {
      ++topic_stats->messages_counts.messages_error;

      throw std::runtime_error{fmt::format(
          "Failed to deliver message to topic '{}' after {} retries",
          topic_name, retries)};
    }
  }
}

void ProducerImpl::Poll(std::chrono::milliseconds poll_timeout) const {
  rd_kafka_poll(producer_->Handle(), static_cast<int>(poll_timeout.count()));
}

ProducerImpl::SendResult ProducerImpl::SendImpl(
    const std::string& topic_name, std::string_view key,
    std::string_view message, std::optional<std::uint32_t> partition) const {
  using enum SendResult;

  auto waiter = std::make_unique<impl::DeliveryWaiter>();
  auto wait_handle = waiter->get_future();

  /// @note `rd_kafka_producev` does not block at all. It only enqueues
  /// the message to the local queue to be send in future by `librdkafka`
  /// internal thread
  /// @note 0 msgflags implies no message copying and freeing by
  /// `librdkafka` implementation. It is safe to pass actually a pointer
  /// to message data because it lives till the delivery callback is invoked
  /// @see
  /// https://github.com/confluentinc/librdkafka/blob/master/src/rdkafka.h#L4698
  /// for understanding of `msgflags` argument
  /// @note It is safe to release the `waiter` because (i)
  /// `rd_kafka_producev` does not throws, therefore it owns the `waiter`,
  /// (ii) delivery report callback fries its memory
  /// @note const qualifier remove for `message` is required because of
  /// the `librdkafka` API requirements. If `msgflags` set to
  /// `RD_KAFKA_MSG_F_FREE`, produce implementation fries the message
  /// data, though not const pointer is required
  const rd_kafka_resp_err_t enqueue_error = rd_kafka_producev(
      producer_->Handle(), RD_KAFKA_V_TOPIC(topic_name.c_str()),
      RD_KAFKA_V_KEY(key.data(), key.size()),
      // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
      RD_KAFKA_V_VALUE(const_cast<char*>(message.data()), message.size()),
      RD_KAFKA_V_MSGFLAGS(0),
      RD_KAFKA_V_PARTITION(partition.value_or(RD_KAFKA_PARTITION_UA)),
      RD_KAFKA_V_OPAQUE(waiter.release()), RD_KAFKA_V_END);

  if (enqueue_error != RD_KAFKA_RESP_ERR_NO_ERROR) {
    LOG_WARNING() << GetLoggingTags()
                  << fmt::format(
                         "Failed to enqueue message to Kafka local queue: {}",
                         rd_kafka_err2str(enqueue_error));

    return IsRetryable(enqueue_error) ? Retryable : Failed;
  }

  /// wait until delivery report callback is invoked:
  /// @see impl/configuration.cpp
  const auto delivery_result = wait_handle.get();
  const auto delivery_error =
      static_cast<rd_kafka_resp_err_t>(delivery_result.message_error);
  const auto message_status =
      static_cast<rd_kafka_msg_status_t>(delivery_result.messages_status);

  if (delivery_error != RD_KAFKA_RESP_ERR_NO_ERROR) {
    return IsRetryable(delivery_error, message_status) ? Retryable : Failed;
  }

  return Succeeded;
}

}  // namespace kafka

USERVER_NAMESPACE_END
