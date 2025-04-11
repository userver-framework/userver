#pragma once

#include <exception>
#include <string>

#include <userver/engine/task/task_with_result.hpp>

USERVER_NAMESPACE_BEGIN

namespace dist_lock::impl {

bool GetTask(
    engine::TaskWithResult<void>& task,
    std::string_view name,
    std::string_view error_context,
    std::exception_ptr* exception_ptr = nullptr
);

std::string LockerName(std::string_view lock_name);

std::string WatchdogName(std::string_view lock_name);

std::string WorkerName(std::string_view lock_name);

}  // namespace dist_lock::impl

USERVER_NAMESPACE_END
