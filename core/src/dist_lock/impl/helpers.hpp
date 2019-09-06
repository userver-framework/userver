#pragma once

#include <string>

#include <engine/task/task_with_result.hpp>

namespace dist_lock::impl {

bool GetTask(engine::TaskWithResult<void>& task, const std::string& name);

std::string LockerName(const std::string& lock_name);

std::string WatchdogName(const std::string& lock_name);

std::string WorkerName(const std::string& lock_name);

}  // namespace dist_lock::impl
