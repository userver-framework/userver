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

  /// Library will create (2 * this value) connections per ev-thread:
  /// in every pair of connections one connection is used for
  /// reliable channels (publisher-confirms), and another one is used for
  /// common channels.
  /// This separation ain't strictly necessary, but you'll have to deal with it.
  ///
  /// You shouldn't set this value too high: 1 is likely enough
  /// for reliable networks, however if your tcp breaks quiet often increasing
  /// this value might reduce latency/error-rate.
  size_t connections_per_thread;

  /// How many channels should library create per connection. I was lazy at the
  /// time of coding the implementation, so this value is a hard limit
  /// of concurrent operations.
  /// TODO : channels should be pooled in a more sophisticated way
  size_t channels_per_connection;
};

}  // namespace urabbitmq

USERVER_NAMESPACE_END