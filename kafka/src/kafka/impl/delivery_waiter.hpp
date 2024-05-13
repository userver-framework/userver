#pragma once

#include <cstdint>

#include <userver/engine/future.hpp>

#include <librdkafka/rdkafka.h>

USERVER_NAMESPACE_BEGIN

namespace kafka::impl {

class DeliveryResult {
 public:
  DeliveryResult(
      rd_kafka_resp_err_t message_error,
      std::optional<rd_kafka_msg_status_t> message_status = std::nullopt);

  /// @note Message delivery may timeout due to timeout fired when request to
  /// broker was in-flight. In that case, message status is
  /// `RD_KAFKA_MSG_STATUS_POSSIBLY_PERSISTED`. It is not safe to retry such
  /// messages, because of reordering and duplication possibility
  /// @see
  /// https://docs.confluent.io/platform/current/clients/librdkafka/html/md_INTRODUCTION.html#autotoc_md21
  bool IsRetryable() const;

  bool IsSuccess() const;

 private:
  const rd_kafka_resp_err_t message_error_;
  const std::optional<rd_kafka_msg_status_t> message_status_;
};

/// @brief State for waiting delivery callback invoked after producer send
/// called
class DeliveryWaiter {
 public:
  DeliveryWaiter(std::uint32_t current_retry, std::uint32_t max_retries);

  engine::Future<DeliveryResult> GetFuture();

  bool FirstSend() const;

  bool LastRetry() const;

  void SetDeliveryResult(DeliveryResult delivery_result);

 private:
  const std::uint32_t current_retry_;
  const std::uint32_t max_retries_;

  engine::Promise<DeliveryResult> wait_handle_;
};

}  // namespace kafka::impl

USERVER_NAMESPACE_END
