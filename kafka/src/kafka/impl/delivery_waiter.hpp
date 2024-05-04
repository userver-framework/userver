#pragma once

#include <userver/engine/future.hpp>

USERVER_NAMESPACE_BEGIN

namespace kafka::impl {

struct DeliveryResult {
  int message_error{};
  int messages_status{};
};

/// @brief State for waiting delivery callback invoked after producer send
/// called
using DeliveryWaiter = engine::Promise<DeliveryResult>;

}  // namespace kafka::impl

USERVER_NAMESPACE_END
