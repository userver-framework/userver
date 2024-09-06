#include <kafka/impl/delivery_waiter.hpp>

USERVER_NAMESPACE_BEGIN

namespace kafka::impl {

DeliveryResult::DeliveryResult(
    rd_kafka_resp_err_t message_error,
    std::optional<rd_kafka_msg_status_t> message_status)
    : message_error_(message_error), message_status_(message_status) {}

bool DeliveryResult::IsSuccess() const {
  return message_error_ == RD_KAFKA_RESP_ERR_NO_ERROR &&
         (message_status_.has_value() &&
          *message_status_ == RD_KAFKA_MSG_STATUS_PERSISTED);
}

rd_kafka_resp_err_t DeliveryResult::GetMessageError() const {
  return message_error_;
}

engine::Future<DeliveryResult> DeliveryWaiter::GetFuture() {
  return wait_handle_.get_future();
}

void DeliveryWaiter::SetDeliveryResult(DeliveryResult delivery_result) {
  wait_handle_.set_value(std::move(delivery_result));
}

}  // namespace kafka::impl

USERVER_NAMESPACE_END
