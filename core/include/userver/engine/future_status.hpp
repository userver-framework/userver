#pragma once

/// @file userver/engine/future_status.hpp
/// @brief @copybrief engine::FutureStatus

USERVER_NAMESPACE_BEGIN

namespace engine {

/// std::future state extended with "cancelled" state
enum class FutureStatus {
  kReady,     ///< the future is ready
  kTimeout,   ///< the wait operation timed out
  kCancelled  ///< the wait operation was interrupted by task cancellation
};

}  // namespace engine

USERVER_NAMESPACE_END
