#include <userver/engine/wait_all_checked.hpp>

#include <algorithm>

#include <engine/impl/wait_any_utils.hpp>
#include <engine/task/task_context.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::impl {

void DoWaitAllChecked(utils::impl::Span<ContextAccessor*> targets) {
  UASSERT_MSG(AreUniqueValues(targets),
              "Same tasks/futures were detected in WaitAny* call");
  auto& current = current_task::GetCurrentTaskContext();

  WaitAnyWaitStrategy wait_strategy(Deadline{}, targets, current);
  while (true) {
    if (current.ShouldCancel()) {
      throw WaitInterruptedException{current.CancellationReason()};
    }

    bool all_completed = true;
    for (auto& target : targets) {
      if (!target) continue;

      const bool is_ready = target->IsReady();
      if (is_ready) {
        target->RethrowErrorResult();
        target = nullptr;
      }
      all_completed &= is_ready;
    }
    if (all_completed) break;

    current.Sleep(wait_strategy);
  }

  UASSERT(std::all_of(targets.begin(), targets.end(),
                      [](auto* target) { return !target; }));
}

}  // namespace engine::impl

USERVER_NAMESPACE_END
