#include <kafka/impl/delivery_waiter.hpp>

USERVER_NAMESPACE_BEGIN

namespace kafka::impl {

DeliveryResult::DeliveryResult(
    rd_kafka_resp_err_t message_error,
    std::optional<rd_kafka_msg_status_t> message_status)
    : message_error_(message_error), message_status_(message_status) {}

bool DeliveryResult::IsRetryable() const {
  const bool retryable_error =
      message_error_ == RD_KAFKA_RESP_ERR__MSG_TIMED_OUT ||
      message_error_ == RD_KAFKA_RESP_ERR__QUEUE_FULL ||
      message_error_ == RD_KAFKA_RESP_ERR_INVALID_REPLICATION_FACTOR;

  const bool retryable_status =
      !message_status_.has_value()
          ? true
          : *message_status_ != RD_KAFKA_MSG_STATUS_PERSISTED &&
                *message_status_ != RD_KAFKA_MSG_STATUS_POSSIBLY_PERSISTED;

  return retryable_error && retryable_status;
}

bool DeliveryResult::IsSuccess() const {
  return message_error_ == RD_KAFKA_RESP_ERR_NO_ERROR &&
         (message_status_.has_value() &&
          *message_status_ == RD_KAFKA_MSG_STATUS_PERSISTED);
}

DeliveryWaiter::DeliveryWaiter(std::uint32_t current_retry,
                               std::uint32_t max_retries)
    : current_retry_(current_retry), max_retries_(max_retries) {}

engine::Future<DeliveryResult> DeliveryWaiter::GetFuture() {
  return wait_handle_.get_future();
}

bool DeliveryWaiter::FirstSend() const { return current_retry_ == 0; }

bool DeliveryWaiter::LastRetry() const {
  return current_retry_ == max_retries_;
}

void DeliveryWaiter::SetDeliveryResult(DeliveryResult delivery_result) {
  wait_handle_.set_value(std::move(delivery_result));
}

}  // namespace kafka::impl

USERVER_NAMESPACE_END
