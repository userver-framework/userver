#pragma once

#include <chrono>

#include <userver/engine/task/inherited_deadline.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::request {

class RequestDeadlineInfo : public engine::TaskInheritedDeadline {
 public:
  RequestDeadlineInfo() = default;
  RequestDeadlineInfo(std::chrono::steady_clock::time_point start_time,
                      engine::Deadline deadline);

  void SetStartTime(std::chrono::steady_clock::time_point start_time);
  std::chrono::steady_clock::time_point GetStartTime() const;

 private:
  std::chrono::steady_clock::time_point start_time_{};
};

void SetCurrentRequestDeadlineInfo(RequestDeadlineInfo deadline_info);

const RequestDeadlineInfo& GetCurrentRequestDeadlineInfo();
const RequestDeadlineInfo* GetCurrentRequestDeadlineInfoUnchecked();

void ResetCurrentRequestDeadlineInfo();

}  // namespace server::request

USERVER_NAMESPACE_END
