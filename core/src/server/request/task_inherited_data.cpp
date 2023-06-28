#include <userver/server/request/task_inherited_data.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::request {

namespace {

TaskInheritedData GetTaskInheritedDataOrDefault() noexcept {
  const auto* const result = kTaskInheritedData.GetOptional();
  return result ? *result : TaskInheritedData{};
}

}  // namespace

engine::Deadline GetTaskInheritedDeadline() noexcept {
  const auto* data = kTaskInheritedData.GetOptional();
  return data ? data->deadline : engine::Deadline{};
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
