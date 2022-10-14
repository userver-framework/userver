#pragma once

#include <variant>

#include <engine/impl/wait_list.hpp>
#include <engine/impl/wait_list_light.hpp>
#include <userver/engine/task/task.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::impl {

class GenericWaitList final {
 public:
  explicit GenericWaitList(Task::WaitMode wait_mode) noexcept;

  void WakeupAll();

  bool IsShared() const noexcept;

 private:
  friend class GenericWaitScope;

  std::variant<WaitListLight, WaitList> waiters_;
};

class GenericWaitScope final {
 public:
  GenericWaitScope(GenericWaitList& owner, TaskContext& context);
  GenericWaitScope(WaitListLight& owner, TaskContext& context);
  GenericWaitScope(WaitList& owner, TaskContext& context);

  GenericWaitScope(GenericWaitScope&&) = delete;
  GenericWaitScope& operator=(GenericWaitScope&&) = delete;
  ~GenericWaitScope();

  void Append() noexcept;
  void Remove() noexcept;

 private:
  std::variant<WaitScopeLight, WaitScope> impl_;
};

}  // namespace engine::impl

USERVER_NAMESPACE_END
