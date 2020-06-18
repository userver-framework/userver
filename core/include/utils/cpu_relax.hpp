#pragma once

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

}  // namespace utils
