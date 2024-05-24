#include <kafka/impl/producer_impl.hpp>

#include <userver/tracing/span.hpp>
#include <userver/utils/trivial_map.hpp>

#include <kafka/impl/configuration.hpp>
#include <kafka/impl/delivery_waiter.hpp>
#include <kafka/impl/error_buffer.hpp>
#include <kafka/impl/stats.hpp>

#include <utility>

USERVER_NAMESPACE_BEGIN

namespace kafka::impl {

namespace {

void ErrorCallback(rd_kafka_t* producer, int error_code, const char* reason,
                   void* opaque_ptr) {
  UASSERT(producer);
  UASSERT(opaque_ptr);

  static_cast<ProducerImpl*>(opaque_ptr)
      ->ErrorCallbackProxy(error_code, reason);
}

void DeliveryReportCallback(rd_kafka_t* producer,
                            const rd_kafka_message_t* message,
                            void* opaque_ptr) {
  UASSERT(producer);
  UASSERT(opaque_ptr);

  static_cast<ProducerImpl*>(opaque_ptr)->DeliveryReportCallbackProxy(message);
}

std::chrono::milliseconds GetMessageLatencySeconds(
    const rd_kafka_message_t* message) {
  const std::chrono::microseconds message_latency_micro{
      rd_kafka_message_latency(message)};

  return std::chrono::duration_cast<std::chrono::milliseconds>(
      message_latency_micro);
}

}  // namespace

ProducerImpl::ProducerHolder::ProducerHolder(rd_kafka_conf_t* conf) {
  ErrorBuffer err_buf;

  handle_ = HandleHolder{
      rd_kafka_new(RD_KAFKA_PRODUCER, conf, err_buf.data(), err_buf.size()),
      &rd_kafka_destroy};
  if (handle_ == nullptr) {
    /// @note `librdkafka` takes ownership on conf iff `rd_kafka_new`
    /// succeeds
    rd_kafka_conf_destroy(conf);

    PrintErrorAndThrow("create producer", err_buf);
  }
}

ProducerImpl::ProducerHolder::~ProducerHolder() {
  if (rd_kafka_flush(Handle(), ProducerImpl::kCoolDownFlushTimeout.count()) ==
      RD_KAFKA_RESP_ERR__TIMED_OUT) {
    LOG_WARNING() << "Producer flushing timeouted on producer destroy. "
                     "Some messages may be not delivered!!!";
  }
}

rd_kafka_t* ProducerImpl::ProducerHolder::Handle() const {
  return handle_.get();
}

void ProducerImpl::ErrorCallbackProxy(int error_code, const char* reason) {
  tracing::Span span{"error_callback"};
  span.AddTag("kafka_callback", "error_callback");

  LOG_ERROR() << fmt::format(
      "Error {} occured because of '{}': {}", error_code, reason,
      rd_kafka_err2str(static_cast<rd_kafka_resp_err_t>(error_code)));

  if (error_code == RD_KAFKA_RESP_ERR__RESOLVE ||
      error_code == RD_KAFKA_RESP_ERR__TRANSPORT ||
      error_code == RD_KAFKA_RESP_ERR__AUTHENTICATION ||
      error_code == RD_KAFKA_RESP_ERR__ALL_BROKERS_DOWN) {
    ++stats_.connections_error;
  }
}

void ProducerImpl::DeliveryReportCallbackProxy(
    const rd_kafka_message_t* message) {
  static constexpr utils::TrivialBiMap kMessageStatus{[](auto selector) {
    return selector()
        .Case(RD_KAFKA_MSG_STATUS_NOT_PERSISTED, "MSG_STATUS_NOT_PERSISTED")
        .Case(RD_KAFKA_MSG_STATUS_POSSIBLY_PERSISTED,
              "MSG_STATUS_POSSIBLY_PERSISTED")
        .Case(RD_KAFKA_MSG_STATUS_PERSISTED, "MSG_STATUS_PERSISTED");
  }};

  tracing::Span span{"delivery_report_callback"};
  span.AddTag("kafka_callback", "delivery_report_callback");

  const char* topic_name = rd_kafka_topic_name(message->rkt);

  auto* complete_handle = static_cast<DeliveryWaiter*>(message->_private);

  auto& topic_stats = stats_.topics_stats[topic_name];
  if (complete_handle->FirstSend()) {
    ++topic_stats->messages_counts.messages_total;
  }

  const auto message_status{rd_kafka_message_status(message)};
  DeliveryResult delivery_result{message->err, message_status};
  const auto message_latency_ms{GetMessageLatencySeconds(message)};

  LOG_DEBUG() << fmt::format(
      "Message delivery report: err: {}, status: {}, latency: {}ms",
      rd_kafka_err2str(message->err),
      kMessageStatus.TryFind(message_status).value_or("<bad status>"),
      message_latency_ms.count());

  topic_stats->avg_ms_spent_time.GetCurrentCounter().Account(
      message_latency_ms.count());

  if (delivery_result.IsSuccess()) {
    ++topic_stats->messages_counts.messages_success;

    LOG_INFO() << fmt::format(
        "Message to topic '{}' delivered successfully to "
        "partition "
        "{} by offset {} in {}ms",
        topic_name, message->partition, message->offset,
        message_latency_ms.count());
  } else if (complete_handle->LastRetry()) {
    ++topic_stats->messages_counts.messages_error;
    LOG_WARNING() << fmt::format("Failed to delivery message to topic '{}': {}",
                                 topic_name, rd_kafka_err2str(message->err));
  }

  complete_handle->SetDeliveryResult(std::move(delivery_result));
  delete complete_handle;
}

ProducerImpl::ProducerImpl(std::unique_ptr<Configuration> configuration)
    : producer_([this, configuration = std::move(configuration)] {
        rd_kafka_conf_t* conf = configuration->Release();
        rd_kafka_conf_set_opaque(conf, this);
        rd_kafka_conf_set_error_cb(conf, &ErrorCallback);
        rd_kafka_conf_set_dr_msg_cb(conf, &DeliveryReportCallback);

        return ProducerHolder{conf};
      }()) {}

ProducerImpl::~ProducerImpl() = default;

void ProducerImpl::Send(const std::string& topic_name, std::string_view key,
                        std::string_view message,
                        std::optional<std::uint32_t> partition,
                        const std::uint32_t max_retries) const {
  LOG_INFO() << fmt::format("Message to topic '{}' is requested to send",
                            topic_name);

  std::optional<DeliveryResult> delivery_result;
  for (std::uint32_t current_retry = 0; current_retry <= max_retries;
       ++current_retry) {
    delivery_result.emplace(SendImpl(topic_name, key, message, partition,
                                     current_retry, max_retries));

    if (delivery_result->IsSuccess()) {
      return;
    }

    if (current_retry == max_retries || !delivery_result->IsRetryable()) {
      throw std::runtime_error{fmt::format(
          "Failed to deliver message to topic '{}' after {} retries",
          topic_name, current_retry)};
    }
    LOG_WARNING() << "Send request failed, but error may be transient, "
                     "retrying..."
                  << fmt::format("(retries left: {})",
                                 max_retries - current_retry);
  }
}

void ProducerImpl::Poll(std::chrono::milliseconds poll_timeout) const {
  rd_kafka_poll(producer_.Handle(), static_cast<int>(poll_timeout.count()));
}

DeliveryResult ProducerImpl::SendImpl(const std::string& topic_name,
                                      std::string_view key,
                                      std::string_view message,
                                      std::optional<std::uint32_t> partition,
                                      std::uint32_t current_retry,
                                      std::uint32_t max_retries) const {
  auto waiter = std::make_unique<DeliveryWaiter>(current_retry, max_retries);
  auto wait_handle = waiter->GetFuture();

  /// `rd_kafka_producev` does not block at all. It only enqueues
  /// the message to the local queue to be send in future by `librdkafka`
  /// internal thread
  ///
  /// 0 msgflags implies no message copying and freeing by
  /// `librdkafka` implementation. It is safe to pass actually a pointer
  /// to message data because it lives till the delivery callback is invoked
  /// @see
  /// https://github.com/confluentinc/librdkafka/blob/master/src/rdkafka.h#L4698
  /// for understanding of `msgflags` argument
  ///
  /// It is safe to release the `waiter` because (i)
  /// `rd_kafka_producev` does not throws, therefore it owns the `waiter`,
  /// (ii) delivery report callback fries its memory
  ///
  /// const qualifier remove for `message` is required because of
  /// the `librdkafka` API requirements. If `msgflags` set to
  /// `RD_KAFKA_MSG_F_FREE`, produce implementation fries the message
  /// data, though not const pointer is required

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wgnu-statement-expression"
#endif

  // NOLINTBEGIN(clang-analyzer-cplusplus.NewDeleteLeaks,cppcoreguidelines-pro-type-const-cast)
  const rd_kafka_resp_err_t enqueue_error = rd_kafka_producev(
      producer_.Handle(), RD_KAFKA_V_TOPIC(topic_name.c_str()),
      RD_KAFKA_V_KEY(key.data(), key.size()),
      RD_KAFKA_V_VALUE(const_cast<char*>(message.data()), message.size()),
      RD_KAFKA_V_MSGFLAGS(0),
      RD_KAFKA_V_PARTITION(partition.value_or(RD_KAFKA_PARTITION_UA)),
      RD_KAFKA_V_OPAQUE(waiter.release()), RD_KAFKA_V_END);
  // NOLINTEND(clang-analyzer-cplusplus.NewDeleteLeaks,cppcoreguidelines-pro-type-const-cast)

#ifdef __clang__
#pragma clang diagnostic pop
#endif

  if (enqueue_error != RD_KAFKA_RESP_ERR_NO_ERROR) {
    LOG_WARNING() << fmt::format(
        "Failed to enqueue message to Kafka local queue: {}",
        rd_kafka_err2str(enqueue_error));

    return DeliveryResult{enqueue_error};
  }

  /// wait until delivery report callback is invoked:
  /// @see DeliveryCallbackProxy
  return wait_handle.get();
}

const Stats& ProducerImpl::GetStats() const { return stats_; }

}  // namespace kafka::impl

USERVER_NAMESPACE_END
