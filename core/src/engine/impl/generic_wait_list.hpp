#pragma once

#include <atomic>
#include <variant>

#include <engine/impl/wait_list.hpp>
#include <engine/impl/wait_list_light.hpp>
#include <userver/engine/task/task.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::impl {

class GenericWaitList final {
 public:
  explicit GenericWaitList(Task::WaitMode wait_mode) noexcept;

  bool GetSignalOrAppend(boost::intrusive_ptr<TaskContext>&& context) noexcept;

  void Remove(TaskContext& context) noexcept;

  void SetSignalAndWakeupAll();

  bool IsSignaled() const noexcept;

  bool IsShared() const noexcept;

 private:
  struct WaitListAndSignal final {
    std::atomic<bool> signal{false};
    WaitList wl{};
  };

  static auto CreateWaitList(Task::WaitMode wait_mode) noexcept;

  std::variant<WaitListLight, WaitListAndSignal> waiters_;
};

}  // namespace engine::impl

USERVER_NAMESPACE_END
