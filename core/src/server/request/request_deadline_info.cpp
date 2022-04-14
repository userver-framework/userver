#include <userver/server/request/request_deadline_info.hpp>

#include <userver/engine/task/inherited_variable.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

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
  engine::SetCurrentTaskInheritedDeadline(
      std::make_unique<RequestDeadlineInfo>(deadline_info));
}

const RequestDeadlineInfo& GetCurrentRequestDeadlineInfo() {
  const auto* result_opt = GetCurrentRequestDeadlineInfoUnchecked();
  UINVARIANT(result_opt, "No request deadline info found in current task");
  return *result_opt;
}

const RequestDeadlineInfo* GetCurrentRequestDeadlineInfoUnchecked() {
  const auto* ptr_opt = engine::GetCurrentTaskInheritedDeadlineUnchecked();
  if (!ptr_opt) return nullptr;
  return dynamic_cast<const RequestDeadlineInfo*>(ptr_opt);
}

void ResetCurrentRequestDeadlineInfo() {
  engine::ResetCurrentTaskInheritedDeadline();
}

}  // namespace server::request

USERVER_NAMESPACE_END
