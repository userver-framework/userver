#pragma once

#include <optional>

#include <engine/impl/generic_wait_list.hpp>
#include <engine/task/task_context.hpp>
#include <userver/engine/impl/wait_many_entry.hpp>
#include <userver/utils/impl/span.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::impl {

class OptionalGenericWaitScope final : public std::optional<GenericWaitScope> {
};

class WaitAnyWaitStrategy final : public WaitStrategy {
 public:
  WaitAnyWaitStrategy(Deadline deadline,
                      utils::impl::Span<WaitManyEntry> targets,
                      TaskContext& current);

  void SetupWakeups() override;
  void DisableWakeups() override;

 private:
  TaskContext& current_;
  const utils::impl::Span<WaitManyEntry> targets_;
  utils::impl::Span<WaitManyEntry> active_targets_;
};

bool AreUniqueValues(utils::impl::Span<WaitManyEntry> targets);

void InitializeWaitScopes(utils::impl::Span<WaitManyEntry> targets,
                          TaskContext& current);

}  // namespace engine::impl

USERVER_NAMESPACE_END
