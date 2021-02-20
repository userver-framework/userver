#include <utils/cpu_relax.hpp>

#include <fmt/format.h>

#include <engine/sleep.hpp>
#include <logging/log.hpp>

namespace utils {

namespace {
constexpr std::chrono::milliseconds kYieldInterval{3};
}

void CpuRelax::RelaxNoScopeTime() { DoRelax(nullptr); }

void CpuRelax::Relax(ScopeTime& scope) { DoRelax(&scope); }

void CpuRelax::DoRelax(ScopeTime* scope) {
  if (every_iterations_ == 0) return;
  if (++iterations_ == every_iterations_) {
    iterations_ = 0;
    const std::string scope_name =
        scope ? scope->CurrentScope() : std::string{};
    if (!scope_name.empty()) scope->Reset();
    LOG_TRACE() << fmt::format("CPU relax: yielding after {} iterations",
                               every_iterations_);
    engine::Yield();
    if (!scope_name.empty()) scope->Reset(scope_name);
  }
}

StreamingCpuRelax::StreamingCpuRelax(std::uint64_t check_time_after_bytes)
    : check_time_after_bytes_(check_time_after_bytes),
      total_bytes_(0),
      bytes_since_last_time_check_(0),
      last_yield_time_(std::chrono::steady_clock::now()) {}

void StreamingCpuRelax::RelaxNoScopeTime(std::uint64_t bytes_processed) {
  bytes_since_last_time_check_ += bytes_processed;

  if (bytes_since_last_time_check_ >= check_time_after_bytes_) {
    total_bytes_ += std::exchange(bytes_since_last_time_check_, 0);

    const auto now = std::chrono::steady_clock::now();

    if (now - last_yield_time_ > kYieldInterval) {
      LOG_TRACE() << "StreamingCpuRelax: yielding after using "
                  << std::chrono::duration_cast<std::chrono::milliseconds>(
                         now - last_yield_time_)
                  << " of CPU time";

      last_yield_time_ = now;
      engine::Yield();
    }
  }
}

std::uint64_t StreamingCpuRelax::GetBytesProcessed() const {
  return total_bytes_ + bytes_since_last_time_check_;
}

}  // namespace utils
