#include <utils/cpu_relax.hpp>

#include <fmt/format.h>

#include <engine/sleep.hpp>
#include <logging/log.hpp>

namespace utils {

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

}  // namespace utils
