#include <userver/engine/wait_any.hpp>

#include <engine/impl/wait_any_utils.hpp>
#include <engine/task/task_context.hpp>
#include <userver/utils/enumerate.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::impl {

std::optional<std::size_t> DoWaitAny(
    utils::impl::Span<ContextAccessor*> targets, Deadline deadline) {
  UASSERT_MSG(AreUniqueValues(targets),
              "Same tasks/futures were detected in WaitAny* call");
  bool none_valid = true;

  for (const auto& [idx, target] : utils::enumerate(targets)) {
    if (!target) continue;
    none_valid = false;
    if (target->IsReady()) return idx;
  }

  if (none_valid) return std::nullopt;

  auto& current = current_task::GetCurrentTaskContext();
  WaitAnyWaitStrategy wait_strategy(deadline, targets, current);
  current.Sleep(wait_strategy);

  for (const auto& [idx, target] : utils::enumerate(targets)) {
    if (target && target->IsReady()) return idx;
  }

  return std::nullopt;
}

}  // namespace engine::impl

USERVER_NAMESPACE_END
