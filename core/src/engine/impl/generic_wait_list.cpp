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

auto CreateWaitScope(std::variant<WaitListLight, WaitList>& owner,
                     TaskContext& context) {
  using ReturnType = std::variant<WaitScopeLight, WaitScope>;
  return std::visit(
      utils::Overloaded{
          [&](WaitListLight& ws) {
            return ReturnType{std::in_place_type<WaitScopeLight>, ws, context};
          },
          [&](WaitList& ws) {
            return ReturnType{std::in_place_type<WaitScope>, ws, context};
          }},
      owner);
}

}  // namespace

GenericWaitList::GenericWaitList(Task::WaitMode wait_mode) noexcept
    : waiters_(CreateWaitList(wait_mode)) {}

void GenericWaitList::WakeupAll() {
  std::visit(utils::Overloaded{[](WaitListLight& ws) { ws.WakeupOne(); },
                               [](WaitList& ws) { ws.WakeupAll(); }},
             waiters_);
}

bool GenericWaitList::IsShared() const noexcept {
  return std::holds_alternative<WaitList>(waiters_);
}

GenericWaitScope::GenericWaitScope(GenericWaitList& owner, TaskContext& context)
    : impl_(CreateWaitScope(owner.waiters_, context)) {}

GenericWaitScope::GenericWaitScope(WaitListLight& owner, TaskContext& context)
    : impl_(std::in_place_type<WaitScopeLight>, owner, context) {}

GenericWaitScope::GenericWaitScope(WaitList& owner, TaskContext& context)
    : impl_(std::in_place_type<WaitScope>, owner, context) {}

GenericWaitScope::~GenericWaitScope() = default;

void GenericWaitScope::Append() noexcept {
  std::visit([](auto& scope) { scope.Append(); }, impl_);
}

void GenericWaitScope::Remove() noexcept {
  std::visit([](auto& scope) { scope.Remove(); }, impl_);
}

}  // namespace engine::impl

USERVER_NAMESPACE_END
