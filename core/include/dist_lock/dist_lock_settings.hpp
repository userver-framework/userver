#pragma once

/// @file dist_lock/dist_lock_settings.hpp
/// @brief @copybrief dist_lock::DistLockSettings

#include <chrono>

/// Distributed locking
namespace dist_lock {

/// Distributed lock settings
struct DistLockSettings {
  /// How often to try to acquire the lock.
  std::chrono::milliseconds acquire_interval{100};

  /// How often to try to prolong the lock while holding the lock.
  std::chrono::milliseconds prolong_interval{100};

  /// For how long to acquire/prolong the lock.
  std::chrono::milliseconds lock_ttl{1000};

  /// How much time we allow for the worker to stop when we're unable to prolong
  /// the lock.
  std::chrono::milliseconds forced_stop_margin{50};

  /// Delay before failed worker_func restart
  std::chrono::milliseconds worker_func_restart_delay{100};
};

}  // namespace dist_lock
