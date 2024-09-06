#pragma once

#include <librdkafka/rdkafka.h>

#include <userver/engine/future.hpp>

USERVER_NAMESPACE_BEGIN

namespace kafka::impl {

class DeliveryResult final {
 public:
  DeliveryResult(
      rd_kafka_resp_err_t message_error,
      std::optional<rd_kafka_msg_status_t> message_status = std::nullopt);

  DeliveryResult(DeliveryResult&&) noexcept = default;

  bool IsSuccess() const;

  rd_kafka_resp_err_t GetMessageError() const;

 private:
  rd_kafka_resp_err_t message_error_;
  std::optional<rd_kafka_msg_status_t> message_status_;
};

/// @brief State for waiting delivery callback invoked after producer send
/// called
class DeliveryWaiter final {
 public:
  DeliveryWaiter() = default;

  engine::Future<DeliveryResult> GetFuture();

  void SetDeliveryResult(DeliveryResult delivery_result);

 private:
  engine::Promise<DeliveryResult> wait_handle_;
};

}  // namespace kafka::impl

USERVER_NAMESPACE_END
