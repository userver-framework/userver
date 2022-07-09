#pragma once

USERVER_NAMESPACE_BEGIN

namespace urabbitmq {

enum class EvPoolType {
  /// With this type `Client` with use the default frameworks ev-pool
  kShared,
  /// With this mode `Client` will create ev-pool and use it
  kOwned
};

struct ClientSettings final {
  /// Whether client should use the default ev thread-pool or create a new one
  EvPoolType ev_pool_type;

  /// How many ev-threads should `Client` create.
  /// Ignored if EvPoolType::kShared is used
  size_t thread_count;
};

}  // namespace urabbitmq

USERVER_NAMESPACE_END