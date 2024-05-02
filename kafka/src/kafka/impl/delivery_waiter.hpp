#pragma once

#include <userver/engine/future.hpp>

USERVER_NAMESPACE_BEGIN

namespace kafka::impl {

/// @brief State for waiting delivery callback invoked after producer send
/// called
using DeliveryWaiter = engine::Promise<int>;

}  // namespace kafka::impl

USERVER_NAMESPACE_END
