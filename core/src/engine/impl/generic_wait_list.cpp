#include "generic_wait_list.hpp"

#include <engine/task/task_context.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/overloaded.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::impl {

namespace {

auto CreateWaitList(Task::WaitMode wait_mode) noexcept {
  using ReturnType = std::variant<WaitListLight, WaitList>;
  switch (wait_mode) {
    case Task::WaitMode::kSingleWaiter:
      return ReturnType{std::in_place_type<WaitListLight>};
    case Task::WaitMode::kMultipleWaiters:
      return ReturnType{std::in_place_type<WaitList>};
  }
  
  UINVARIANT(false, "Unexpected Task::WaitMode");
}

}  // namespace

GenericWaitList::GenericWaitList(Task::WaitMode wait_mode) noexcept
    : waiters_(CreateWaitList(wait_mode)) {}

// waiters_ are never valueless_by_exception
// NOLINTNEXTLINE(bugprone-exception-escape)
void GenericWaitList::Append(
    boost::intrusive_ptr<TaskContext> context) noexcept {
  std::visit(utils::Overloaded{[&context](WaitListLight& ws) {
                                 ws.Append(std::move(context));
                               },
                               [&context](WaitList& ws) {
                                 WaitList::Lock lock{ws};
                                 ws.Append(lock, std::move(context));
                               }},
             waiters_);
}

// waiters_ are never valueless_by_exception
// NOLINTNEXTLINE(bugprone-exception-escape)
void GenericWaitList::Remove(impl::TaskContext& context) noexcept {
  std::visit(
      utils::Overloaded{[&context](WaitListLight& ws) { ws.Remove(context); },
                        [&context](WaitList& ws) {
                          WaitList::Lock lock{ws};
                          ws.Remove(lock, context);
                        }},
      waiters_);
}

void GenericWaitList::WakeupAll() {
  std::visit(utils::Overloaded{[](WaitListLight& ws) { ws.WakeupOne(); },
                               [](WaitList& ws) {
                                 WaitList::Lock lock{ws};
                                 ws.WakeupAll(lock);
                               }},
             waiters_);
}

bool GenericWaitList::IsShared() const noexcept {
  return std::holds_alternative<WaitList>(waiters_);
}

}  // namespace engine::impl

USERVER_NAMESPACE_END
