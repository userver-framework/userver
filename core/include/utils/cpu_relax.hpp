#pragma once

#include <chrono>
#include <cstddef>

#include <utils/prof.hpp>

namespace utils {

/// Utility to yield every N iterations in a CPU-bound task to give other tasks
/// an opportunity to get CPU time
class CpuRelax {
 public:
  /// @param every number of iterations to call yield. 0 = noop
  explicit CpuRelax(std::size_t every) noexcept
      : every_iterations_{every}, iterations_{0} {}
  CpuRelax(const CpuRelax&) = delete;
  CpuRelax(CpuRelax&&) = delete;

  /// Call this method every iteration, eventually it will yield
  void RelaxNoScopeTime();

  /// Call this method every iteration, eventually it will yield
  /// When yielding it will stop the scope time and then start it again
  void Relax(ScopeTime&);

 private:
  void DoRelax(ScopeTime*);

  const std::size_t every_iterations_;
  std::size_t iterations_;
};

/// Utility to yield in a CPU-bound data processing task
/// to give other tasks an opportunity to get CPU time
class StreamingCpuRelax {
 public:
  /// Constructs with specified `check_time_after_bytes`
  explicit StreamingCpuRelax(std::uint64_t check_time_after_bytes);

  /// Checks time and potentially calls `engine::Yield()`
  /// each `check_time_after_bytes` bytes
  void RelaxNoScopeTime(std::uint64_t bytes_processed);

  /// Returns the total amount of bytes processed since the creation
  /// of `StreamingCpuRelax`
  std::uint64_t GetBytesProcessed() const;

 private:
  std::uint64_t check_time_after_bytes_;
  std::uint64_t total_bytes_;
  std::uint64_t bytes_since_last_time_check_;
  std::chrono::steady_clock::time_point last_yield_time_;
};

}  // namespace utils
