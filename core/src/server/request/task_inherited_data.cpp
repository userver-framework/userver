#include <userver/server/request/task_inherited_data.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::request {

namespace {

TaskInheritedData GetTaskInheritedDataOrDefault() noexcept {
  const auto* const result = kTaskInheritedData.GetOptional();
  return result ? *result : TaskInheritedData{};
}

}  // namespace

DeadlineSignal::DeadlineSignal() noexcept = default;

DeadlineSignal::DeadlineSignal(const DeadlineSignal& other) noexcept
    : value_(other.value_.load(std::memory_order_relaxed)) {}

DeadlineSignal& DeadlineSignal::operator=(
    const DeadlineSignal& other) noexcept {
  if (this == &other) return *this;
  value_.store(other.value_.load(std::memory_order_relaxed),
               std::memory_order_relaxed);
  return *this;
}

void DeadlineSignal::SetExpired() noexcept {
  value_.store(true, std::memory_order_relaxed);
}

bool DeadlineSignal::IsExpired() const noexcept {
  return value_.load(std::memory_order_relaxed);
}

engine::TaskInheritedVariable<TaskInheritedData> kTaskInheritedData;

engine::Deadline GetTaskInheritedDeadline() noexcept {
  const auto* data = kTaskInheritedData.GetOptional();
  return data ? data->deadline : engine::Deadline{};
}

void MarkTaskInheritedDeadlineExpired() noexcept {
  const auto* data = kTaskInheritedData.GetOptional();
  if (data) {
    data->deadline_signal.SetExpired();
  }
}

DeadlinePropagationBlocker::DeadlinePropagationBlocker()
    : old_value_(GetTaskInheritedDataOrDefault()) {
  if (old_value_.deadline.IsReachable()) {
    auto patched = old_value_;
    patched.deadline = {};
    kTaskInheritedData.Set(std::move(patched));
  }
}

DeadlinePropagationBlocker::~DeadlinePropagationBlocker() {
  if (old_value_.deadline.IsReachable()) {
    kTaskInheritedData.Set(std::move(old_value_));
  }
}

}  // namespace server::request

USERVER_NAMESPACE_END
