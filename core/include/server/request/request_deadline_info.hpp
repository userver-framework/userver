#pragma once

#include <chrono>

#include <engine/deadline.hpp>

namespace server::request {

struct RequestDeadlineInfo {
  std::chrono::steady_clock::time_point start_time{};
  engine::Deadline deadline{};
};

void SetCurrentRequestDeadlineInfo(RequestDeadlineInfo deadline_info);

const RequestDeadlineInfo& GetCurrentRequestDeadlineInfo();
const RequestDeadlineInfo* GetCurrentRequestDeadlineInfoUnchecked();

}  // namespace server::request
