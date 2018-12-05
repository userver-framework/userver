#pragma once

/// @file storages/mongo/pool_config.hpp
/// @brief @copybrief storages::mongo::PoolConfig

#include <cstddef>

#include <components/component_config.hpp>

namespace storages {
namespace mongo {

/// MongoDB connection pool configuration
class PoolConfig {
 public:
  /// Default connection timeout
  static constexpr int kDefaultConnTimeoutMs = 5 * 1000;
  /// Default socket timeout
  static constexpr int kDefaultSoTimeoutMs = 30 * 1000;
  /// Default initial connection count
  static constexpr size_t kDefaultMinSize = 32;
  /// Default idle connections limit
  static constexpr size_t kDefaultMaxSize = 256;
  /// Overall connection limit (fixed)
  static constexpr size_t kCriticalSize = 1000;

  /// Constructor for component use
  explicit PoolConfig(const components::ComponentConfig&);

  /// Connection timeout (ms)
  const int conn_timeout_ms;
  /// Socket (I/O) timeout (ms)
  const int so_timeout_ms;
  /// Initial connection count
  const size_t min_size;
  /// Idle connections limit
  const size_t max_size;
};

}  // namespace mongo
}  // namespace storages
