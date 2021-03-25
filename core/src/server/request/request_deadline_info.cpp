#include <server/request/request_deadline_info.hpp>

#include <utils/assert.hpp>
#include <utils/task_inherited_data.hpp>

namespace server::request {

namespace {
const std::string kCurrentRequestDeadlineInfo = "current-request-deadline-info";
}  // namespace

void SetCurrentRequestDeadlineInfo(RequestDeadlineInfo deadline_info) {
  ::utils::SetTaskInheritedData(kCurrentRequestDeadlineInfo, deadline_info);
}

const RequestDeadlineInfo& GetCurrentRequestDeadlineInfo() {
  auto result_opt = GetCurrentRequestDeadlineInfoUnchecked();
  YTX_INVARIANT(result_opt, "No request deadline info found in current task");
  return *result_opt;
}

const RequestDeadlineInfo* GetCurrentRequestDeadlineInfoUnchecked() {
  return ::utils::GetTaskInheritedDataOptional<RequestDeadlineInfo>(
      kCurrentRequestDeadlineInfo);
}

}  // namespace server::request
