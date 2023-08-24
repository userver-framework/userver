#include "generic_wait_list.hpp"

#include <engine/task/task_context.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/overloaded.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::impl {

namespace {

auto CreateWaitList(Task::WaitMode wait_mode) noexcept {
  using ReturnType = std::variant<WaitListLight, WaitList>;
  if (wait_mode == Task::WaitMode::kSingleWaiter) {
    return ReturnType{std::in_place_type<WaitListLight>};
  }

  UASSERT_MSG(wait_mode == Task::WaitMode::kMultipleWaiters,
              "Unexpected Task::WaitMode");
  return ReturnType{std::in_place_type<WaitList>};
}

}  // namespace

GenericWaitList::GenericWaitList(Task::WaitMode wait_mode) noexcept
    : waiters_(CreateWaitList(wait_mode)) {}

// noexcept: waiters_ are never valueless_by_exception
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

// noexcept: waiters_ are never valueless_by_exception
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
