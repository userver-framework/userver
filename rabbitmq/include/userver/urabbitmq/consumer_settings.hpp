#pragma once

#include <cstddef>

#include <userver/urabbitmq/typedefs.hpp>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq {

/// Consumer settings class
struct ConsumerSettings final {
  /// A queue to consume from
  Queue queue;

  /// Limit for unacked messages, degree of parallelism if you will.
  std::uint16_t prefetch_count;
};

}  // namespace urabbitmq

USERVER_NAMESPACE_END
