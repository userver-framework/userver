#pragma once

/// @file storages/mongo_ng/pool_config.hpp
/// @brief @copybrief storages::mongo_ng::PoolConfig

#include <cstddef>
#include <string>

namespace storages::mongo_ng {

/// MongoDB connection pool configuration
class PoolConfig {
 public:
  /// Default connection timeout
  static constexpr int kDefaultConnTimeoutMs = 5 * 1000;
  /// Default socket timeout
  static constexpr int kDefaultSoTimeoutMs = 30 * 1000;
  /// Default initial connection count
  static constexpr size_t kDefaultInitialSize = 32;
  /// Default idle connections limit
  static constexpr size_t kDefaultIdleLimit = 256;
  /// Overall connection limit (fixed)
  static constexpr size_t kMaxSize = 1000;

  // Constructor for component use
  // explicit PoolConfig(const components::ComponentConfig&);

  /// @cond
  // Constructs a constrained pool for tests, not to be used in production code
  explicit PoolConfig(std::string app_name);
  /// @endcond

  /// Connection timeout (ms)
  const int conn_timeout_ms;
  /// Socket (I/O) timeout (ms)
  const int so_timeout_ms;
  /// Initial connection count
  const size_t initial_size;
  /// Idle connections limit
  const size_t idle_limit;

  /// Application name (sent to server)
  const std::string app_name;
};

}  // namespace storages::mongo_ng
