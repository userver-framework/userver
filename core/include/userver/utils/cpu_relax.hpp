#pragma once

#include <chrono>
#include <cstddef>

#include <utils/prof.hpp>

namespace utils {

/// Utility to easily pause ScopeTime, e.g. when yielding
class ScopeTimePause {
 public:
  explicit ScopeTimePause(ScopeTime* scope);

  /// Pause a ScopeTime, if any
  void Pause();

  /// Unpause the ScopeTime, if any
  void Unpause();

 private:
  ScopeTime* scope_;
  std::string scope_name_;
};

/// Utility to yield every N iterations in a CPU-bound task to give other tasks
/// an opportunity to get CPU time
class CpuRelax {
 public:
  /// @param every number of iterations to call yield. 0 = noop
  /// @param scope the ScopeTime to pause when yielding, if any
  /// @warning The `ScopeTime` must live at least as long as the `CpuRelax`
  explicit CpuRelax(std::size_t every, ScopeTime* scope);

  CpuRelax(const CpuRelax&) = delete;
  CpuRelax(CpuRelax&&) = delete;

  /// Call this method every iteration, eventually it will yield
  void Relax();

 private:
  ScopeTimePause pause_;
  const std::size_t every_iterations_;
  std::size_t iterations_;
};

/// Utility to yield in a CPU-bound data processing task
/// to give other tasks an opportunity to get CPU time
class StreamingCpuRelax {
 public:
  /// @param check_time_after_bytes number of bytes to call yield
  /// @param scope the ScopeTime to pause when yielding, if any
  /// @warning The `ScopeTime` must live at least as long as the `CpuRelax`
  explicit StreamingCpuRelax(std::uint64_t check_time_after_bytes,
                             ScopeTime* scope);

  /// Checks time and potentially calls `engine::Yield()`
  /// each `check_time_after_bytes` bytes
  void Relax(std::uint64_t bytes_processed);

  /// Returns the total amount of bytes processed since the creation
  /// of `StreamingCpuRelax`
  std::uint64_t GetBytesProcessed() const;

 private:
  ScopeTimePause pause_;
  std::uint64_t check_time_after_bytes_;
  std::uint64_t total_bytes_;
  std::uint64_t bytes_since_last_time_check_;
  std::chrono::steady_clock::time_point last_yield_time_;
};

}  // namespace utils
