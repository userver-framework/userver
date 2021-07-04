#include <userver/server/request/request_deadline_info.hpp>

#include <userver/utils/assert.hpp>
#include <userver/utils/task_inherited_data.hpp>

namespace server::request {

RequestDeadlineInfo::RequestDeadlineInfo(
    std::chrono::steady_clock::time_point start_time, engine::Deadline deadline)
    : engine::TaskInheritedDeadline(deadline), start_time_(start_time) {}

void RequestDeadlineInfo::SetStartTime(
    std::chrono::steady_clock::time_point start_time) {
  start_time_ = start_time;
}

std::chrono::steady_clock::time_point RequestDeadlineInfo::GetStartTime()
    const {
  return start_time_;
}

void SetCurrentRequestDeadlineInfo(RequestDeadlineInfo deadline_info) {
  ::utils::EmplaceTaskInheritedData<
      std::unique_ptr<engine::TaskInheritedDeadline>>(
      engine::kTaskInheritedDeadlineKey,
      std::make_unique<RequestDeadlineInfo>(deadline_info));
}

const RequestDeadlineInfo& GetCurrentRequestDeadlineInfo() {
  auto result_opt = GetCurrentRequestDeadlineInfoUnchecked();
  YTX_INVARIANT(result_opt, "No request deadline info found in current task");
  return *result_opt;
}

const RequestDeadlineInfo* GetCurrentRequestDeadlineInfoUnchecked() {
  auto ptr_opt = ::utils::GetTaskInheritedDataOptional<
      std::unique_ptr<engine::TaskInheritedDeadline>>(
      engine::kTaskInheritedDeadlineKey);
  if (!ptr_opt) return nullptr;
  return dynamic_cast<const RequestDeadlineInfo*>(ptr_opt->get());
}

void ResetCurrentRequestDeadlineInfo() {
  engine::ResetCurrentTaskInheritedDeadline();
}

}  // namespace server::request
