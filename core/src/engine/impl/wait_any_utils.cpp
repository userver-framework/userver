#include <engine/impl/wait_any_utils.hpp>

#include <algorithm>
#include <utility>
#include <vector>

#include <boost/range/adaptor/filtered.hpp>
#include <boost/range/adaptor/transformed.hpp>

#include <userver/engine/impl/context_accessor.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/lazy_prvalue.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::impl {

WaitAnyWaitStrategy::WaitAnyWaitStrategy(
    Deadline deadline, utils::impl::Span<WaitManyEntry> targets,
    TaskContext& current)
    : WaitStrategy(deadline), current_(current), targets_(targets) {}

void WaitAnyWaitStrategy::SetupWakeups() {
  for (auto& target : targets_) {
    UASSERT((target.context_accessor != nullptr) ==
            target.wait_scope->has_value());

    if (!target.context_accessor) continue;

    (**target.wait_scope).Append();

    if (target.context_accessor->IsReady()) {
      active_targets_ = {targets_.begin(), &target + 1};
      current_.Wakeup(TaskContext::WakeupSource::kWaitList,
                      TaskContext::NoEpoch{});
      return;
    }
  }

  active_targets_ = targets_;
}

void WaitAnyWaitStrategy::DisableWakeups() {
  for (auto& target : active_targets_) {
    if (!target.context_accessor) continue;

    (**target.wait_scope).Remove();
  }
}

bool AreUniqueValues(utils::impl::Span<WaitManyEntry> targets) {
  auto context_accessors =
      targets | boost::adaptors::transformed([](const auto& target) {
        return target.context_accessor;
      }) |
      boost::adaptors::filtered([](auto it) { return it != nullptr; });
  std::vector<ContextAccessor*> sorted(context_accessors.begin(),
                                       context_accessors.end());
  std::sort(sorted.begin(), sorted.end());
  return std::adjacent_find(sorted.begin(), sorted.end()) == sorted.end();
}

void InitializeWaitScopes(utils::impl::Span<WaitManyEntry> targets,
                          TaskContext& current) {
  for (auto& target : targets) {
    if (target.context_accessor) {
      target.wait_scope->emplace(utils::LazyPrvalue(
          [&] { return target.context_accessor->MakeWaitScope(current); }));
    } else {
      target.wait_scope->reset();
    }
  }
}

}  // namespace engine::impl

USERVER_NAMESPACE_END
