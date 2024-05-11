#include <kafka/impl/producer_impl.hpp>

#include <userver/logging/log_extra.hpp>
#include <userver/utils/trivial_map.hpp>

#include <kafka/impl/configuration.hpp>
#include <kafka/impl/delivery_waiter.hpp>
#include <kafka/impl/error_buffer.hpp>
#include <kafka/impl/stats.hpp>

#include <rdkafka.h>

#include <utility>

USERVER_NAMESPACE_BEGIN

namespace kafka::impl {

class ProducerImpl::ProducerHolder final {
 public:
  ProducerHolder(void* conf_ptr)
      : handle_(rd_kafka_new(RD_KAFKA_PRODUCER,
                             static_cast<rd_kafka_conf_t*>(conf_ptr),
                             err_buf_.data(), err_buf_.size()),
                &rd_kafka_destroy) {
    if (handle_ == nullptr) {
      /// @note `librdkafka` takes ownership on conf iff `rd_kafka_new`
      /// succeeds
      rd_kafka_conf_destroy(static_cast<rd_kafka_conf_t*>(conf_ptr));

      PrintErrorAndThrow("create producer", err_buf_);
    }
  }

  ~ProducerHolder() {
    if (rd_kafka_flush(Handle(), ProducerImpl::kCoolDownFlushTimeout.count()) ==
        RD_KAFKA_RESP_ERR__TIMED_OUT) {
      LOG_WARNING() << "Producer flushing timeouted on producer destroy. "
                       "Some messages may be not delivered!!!";
    }
  }

  rd_kafka_t* Handle() const { return handle_.get(); }

 private:
  ErrorBuffer err_buf_{};

  using HandleHolder = std::unique_ptr<rd_kafka_t, decltype(&rd_kafka_destroy)>;
  HandleHolder handle_;
};

namespace {

/// @note Message delivery may timeout due to timeout fired when request to
/// broker was in-flight. In that case, message status is
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

/// @param message represents the delivered (or not) message. Its `_private`
/// field contains for `opaque` argument, which was passed to
/// `rd_kafka_producev`
/// @param opaque A global opaque parameter which is equal for all callbacks
/// and is set with `rd_kafka_conf_set_opaque`
void DeliveryReportCallback([[maybe_unused]] rd_kafka_t* producer_,
                            const rd_kafka_message_t* message,
                            void* opaque_ptr) {
  static constexpr utils::TrivialBiMap kMessageStatus{[](auto selector) {
    return selector()
        .Case(RD_KAFKA_MSG_STATUS_NOT_PERSISTED, "MSG_STATUS_NOT_PERSISTED")
        .Case(RD_KAFKA_MSG_STATUS_POSSIBLY_PERSISTED,
              "MSG_STATUS_POSSIBLY_PERSISTED")
        .Case(RD_KAFKA_MSG_STATUS_PERSISTED, "MSG_STATUS_PERSISTED");
  }};

  const auto& opaque = Opaque::FromPtr(opaque_ptr);
  const auto log_tags = opaque.MakeCallbackLogTags("offset_commit_callback");

  const auto message_status{rd_kafka_message_status(message)};
  DeliveryResult delivery_result{static_cast<int>(message->err),
                                 static_cast<int>(message_status)};
  LOG_DEBUG()
      << log_tags
      << fmt::format(
             "Message delivery report: err: {}, status: {}",
             rd_kafka_err2str(message->err),
             kMessageStatus.TryFind(message_status).value_or("<bad status>"));

  auto* complete_handle = static_cast<DeliveryWaiter*>(message->_private);
  complete_handle->set_value(std::move(delivery_result));
  delete complete_handle;

  const char* topic_name = rd_kafka_topic_name(message->rkt);

  if (message->err) {
    LOG_WARNING() << log_tags
                  << fmt::format("Failed to delivery message to topic '{}': {}",
                                 topic_name, rd_kafka_err2str(message->err));
    return;
  }

  LOG_INFO() << log_tags
             << fmt::format(
                    "Message to topic '{}' delivered successfully to "
                    "partition "
                    "{} by offset {}",
                    topic_name, message->partition, message->offset);
}

}  // namespace

ProducerImpl::ProducerImpl(std::unique_ptr<Configuration> configuration)
    : opaque_(configuration->GetComponentName(), EntityType::kProducer),
      producer_(std::make_unique<ProducerHolder>(
          configuration->SetOpaque(opaque_)
              .SetCallbacks([](void* conf) {
                rd_kafka_conf_set_dr_msg_cb(static_cast<rd_kafka_conf_t*>(conf),
                                            &DeliveryReportCallback);
              })
              .Release())) {}

ProducerImpl::~ProducerImpl() = default;

void ProducerImpl::Send(const std::string& topic_name, std::string_view key,
                        std::string_view message,
                        std::optional<std::uint32_t> partition,
                        std::size_t retries) const {
  LOG_INFO() << fmt::format("Message to topic '{}' is requested to send",
                            topic_name);

  auto& topic_stats = opaque_.GetStats().topics_stats[topic_name];
  ++topic_stats->messages_counts.messages_total;

  const auto produce_start_time =
      std::chrono::system_clock::now().time_since_epoch();

  std::size_t retries_left{retries};
  const auto ShouldRetry = [&retries_left](SendResult send_result) {
    if (retries_left > 0 && send_result == SendResult::Retryable) {
      LOG_WARNING() << "Send request failed, but error may be transient, "
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

  LOG_INFO() << fmt::format(
      "Message produced to topic '{}' in {}ms {}", topic_name,
      produce_duration_ms,
      retries > retries_left
          ? fmt::format("(after {} retries)", retries - retries_left)
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
  auto waiter = std::make_unique<DeliveryWaiter>();
  auto wait_handle = waiter->get_future();

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
  const rd_kafka_resp_err_t enqueue_error = rd_kafka_producev(
      producer_->Handle(), RD_KAFKA_V_TOPIC(topic_name.c_str()),
      RD_KAFKA_V_KEY(key.data(), key.size()),
      // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
      RD_KAFKA_V_VALUE(const_cast<char*>(message.data()), message.size()),
      RD_KAFKA_V_MSGFLAGS(0),
      RD_KAFKA_V_PARTITION(partition.value_or(RD_KAFKA_PARTITION_UA)),
      RD_KAFKA_V_OPAQUE(waiter.release()), RD_KAFKA_V_END);

  if (enqueue_error != RD_KAFKA_RESP_ERR_NO_ERROR) {
    LOG_WARNING() << fmt::format(
        "Failed to enqueue message to Kafka local queue: {}",
        rd_kafka_err2str(enqueue_error));

    return IsRetryable(enqueue_error) ? SendResult::Retryable
                                      : SendResult::Failed;
  }

  /// wait until delivery report callback is invoked:
  /// @see DeliveryCallback
  const auto delivery_result = wait_handle.get();
  const auto delivery_error =
      static_cast<rd_kafka_resp_err_t>(delivery_result.message_error);
  const auto message_status =
      static_cast<rd_kafka_msg_status_t>(delivery_result.messages_status);

  if (delivery_error != RD_KAFKA_RESP_ERR_NO_ERROR) {
    return IsRetryable(delivery_error, message_status) ? SendResult::Retryable
                                                       : SendResult::Failed;
  }

  return SendResult::Succeeded;
}

const Stats& ProducerImpl::GetStats() const { return opaque_.GetStats(); }

}  // namespace kafka::impl

USERVER_NAMESPACE_END
