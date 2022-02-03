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

  // NOLINTNEXTLINE(bugprone-exception-escape)
  void Append(boost::intrusive_ptr<TaskContext> context) noexcept;

  // NOLINTNEXTLINE(bugprone-exception-escape)
  void Remove(impl::TaskContext& context) noexcept;

  void WakeupAll();

  bool IsShared() const noexcept;

 private:
  std::variant<WaitListLight, WaitList> waiters_;
};

}  // namespace engine::impl

USERVER_NAMESPACE_END
