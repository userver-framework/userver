#pragma once

/// @file dist_lock/dist_lock_strategy.hpp
/// @brief @copybrief dist_lock::DistLockStrategyBase

#include <chrono>
#include <exception>

namespace dist_lock {

/// Indicates that lock cannot be acquired because it's busy.
class LockIsAcquiredByAnotherHostException : public std::exception {};

/// Interface for distributed lock strategies
class DistLockStrategyBase {
 public:
  virtual ~DistLockStrategyBase() {}

  /// Acquires the distributed lock.
  /// @throws LockIsAcqiredByAnotherHostError when the lock is busy
  /// @throws anything else when the locking fails, strategy is responsible for
  /// cleanup, Release won't be invoked.
  virtual void Acquire(std::chrono::milliseconds) = 0;

  /// Releases the lock.
  /// @note Exceptions are ignored.
  virtual void Release() = 0;
};

}  // namespace dist_lock
