#include <userver/utils/cpu_relax.hpp>

#include <fmt/format.h>

#include <userver/engine/sleep.hpp>
#include <userver/engine/task/task.hpp>
#include <userver/logging/log.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils {

namespace {
constexpr std::chrono::milliseconds kYieldInterval{3};
}  // namespace

ScopeTimePause::ScopeTimePause(tracing::ScopeTime* scope) : scope_(scope) {}

void ScopeTimePause::Pause() {
  if (scope_) {
    scope_name_ = scope_->CurrentScope();
    scope_->Reset();
  }
}

void ScopeTimePause::Unpause() {
  if (scope_) {
    scope_->Reset(std::move(scope_name_));
    scope_name_.clear();
  }
}

CpuRelax::CpuRelax(std::size_t every, tracing::ScopeTime* scope)
    : pause_(scope), every_iterations_(every) {}

void CpuRelax::Relax() {
  if (every_iterations_ == 0) return;
  if (++iterations_ == every_iterations_) {
    iterations_ = 0;
    pause_.Pause();
    LOG_TRACE() << fmt::format("CPU relax: yielding after {} iterations",
                               every_iterations_);
    if (engine::current_task::GetTaskProcessorOptional()) {
      engine::Yield();
    }
    pause_.Unpause();
  }
}

StreamingCpuRelax::StreamingCpuRelax(std::uint64_t check_time_after_bytes,
                                     tracing::ScopeTime* scope)
    : pause_(scope),
      check_time_after_bytes_(check_time_after_bytes),
      last_yield_time_(std::chrono::steady_clock::now()) {
  UASSERT(check_time_after_bytes != 0);
}

void StreamingCpuRelax::Relax(std::uint64_t bytes_processed) {
  bytes_since_last_time_check_ += bytes_processed;

  if (bytes_since_last_time_check_ >= check_time_after_bytes_) {
    total_bytes_ += std::exchange(bytes_since_last_time_check_, 0);

    const auto now = std::chrono::steady_clock::now();

    if (now - last_yield_time_ > kYieldInterval) {
      pause_.Pause();
      LOG_TRACE() << "StreamingCpuRelax: yielding after using "
                  << std::chrono::duration_cast<std::chrono::milliseconds>(
                         now - last_yield_time_)
                  << " of CPU time";

      last_yield_time_ = now;
      if (engine::current_task::GetTaskProcessorOptional()) {
        engine::Yield();
      }
      pause_.Unpause();
    }
  }
}

std::uint64_t StreamingCpuRelax::GetBytesProcessed() const {
  return total_bytes_ + bytes_since_last_time_check_;
}

}  // namespace utils

USERVER_NAMESPACE_END
