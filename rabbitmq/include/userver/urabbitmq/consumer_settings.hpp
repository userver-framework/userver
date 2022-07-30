#pragma once

/// @file userver/urabbitmq/consumer_settings.hpp
/// @brief Consumer settings.

#include <cstddef>

#include <userver/urabbitmq/typedefs.hpp>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq {

/// @brief Consumer settings struct
struct ConsumerSettings final {
  /// A queue to consume from
  Queue queue;

  /// Limit for unacked messages, degree of parallelism if you will.
  /// Tasks are dispatched asynchronously, so be a bit careful with this value:
  /// up to this value of tasks might be executed concurrently,
  /// which could be overwhelming
  ///
  /// Settings this value to 1 basically makes a consumer synchronous, which
  /// could be of use for some workloads
  std::uint16_t prefetch_count;
};

}  // namespace urabbitmq

USERVER_NAMESPACE_END
