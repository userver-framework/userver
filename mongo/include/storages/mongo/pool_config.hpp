#pragma once

/// @file storages/mongo/pool_config.hpp
/// @brief @copybrief storages::mongo::PoolConfig

#include <chrono>
#include <cstddef>
#include <optional>
#include <string>

#include <components/component_config.hpp>

namespace storages::mongo {

/// MongoDB connection pool configuration
class PoolConfig {
 public:
  /// Default connection timeout
  static constexpr auto kDefaultConnTimeout = std::chrono::seconds{5};
  /// Default socket timeout
  static constexpr auto kDefaultSoTimeout = std::chrono::seconds{10};
  /// Default connection queue timeout
  static constexpr auto kDefaultQueueTimeout = std::chrono::seconds{1};
  /// Default initial connection count
  static constexpr size_t kDefaultInitialSize = 16;
  /// Default total connections limit
  static constexpr size_t kDefaultMaxSize = 128;
  /// Default idle connections limit
  static constexpr size_t kDefaultIdleLimit = 64;
  /// Default establishing connections limit
  static constexpr size_t kDefaultConnectingLimit = 8;
  /// Default pool maintenance period
  static constexpr auto kDefaultMaintenancePeriod = std::chrono::seconds{15};

  // Constructor for component use
  explicit PoolConfig(const components::ComponentConfig&);

  /// @cond
  // Constructs a constrained pool for tests, not to be used in production code
  explicit PoolConfig(std::string app_name);
  /// @endcond

  /// Connection (I/O) timeout
  std::chrono::milliseconds conn_timeout;
  /// Socket (I/O) timeout
  std::chrono::milliseconds so_timeout;
  /// Connection queue wait time
  std::chrono::milliseconds queue_timeout;
  /// Initial connection count
  size_t initial_size;
  /// Total connections limit
  size_t max_size;
  /// Idle connections limit
  size_t idle_limit;
  /// Establishing connections limit
  size_t connecting_limit;
  /// Instance selection latency window override
  std::optional<std::chrono::milliseconds> local_threshold;
  /// Pool maintenance period
  std::chrono::milliseconds maintenance_period;

  /// Application name (sent to server)
  const std::string app_name;
};

}  // namespace storages::mongo
