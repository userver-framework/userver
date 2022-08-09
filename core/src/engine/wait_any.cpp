#include <userver/engine/wait_any.hpp>

#include <engine/impl/wait_any_utils.hpp>
#include <engine/task/task_context.hpp>
#include <userver/utils/enumerate.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::impl {

std::optional<std::size_t> DoWaitAny(utils::impl::Span<WaitManyEntry> targets,
                                     Deadline deadline) {
  UASSERT_MSG(AreUniqueValues(targets),
              "Same tasks/futures were detected in WaitAny* call");
  bool none_valid = true;

  for (const auto& [idx, target] : utils::enumerate(targets)) {
    if (!target.context_accessor) continue;
    none_valid = false;
    if (target.context_accessor->IsReady()) return idx;
  }

  if (none_valid) return std::nullopt;

  auto& current = current_task::GetCurrentTaskContext();
  if (current.ShouldCancel()) {
    throw WaitInterruptedException(current.CancellationReason());
  }

  InitializeWaitScopes(targets, current);
  WaitAnyWaitStrategy wait_strategy(deadline, targets, current);
  current.Sleep(wait_strategy);

  for (const auto& [idx, target] : utils::enumerate(targets)) {
    if (target.context_accessor && target.context_accessor->IsReady()) {
      return idx;
    }
  }

  return std::nullopt;
}

}  // namespace engine::impl

USERVER_NAMESPACE_END
