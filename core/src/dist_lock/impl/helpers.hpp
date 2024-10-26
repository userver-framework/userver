#pragma once

#include <exception>
#include <string>

#include <userver/engine/task/task_with_result.hpp>
#include <userver/utils/impl/source_location.hpp>

USERVER_NAMESPACE_BEGIN

namespace dist_lock::impl {

bool GetTask(
    engine::TaskWithResult<void>& task,
    std::string_view name,
    std::exception_ptr* exception_ptr = nullptr,
    utils::impl::SourceLocation location = utils::impl::SourceLocation::Current()
);

std::string LockerName(std::string_view lock_name);

std::string WatchdogName(std::string_view lock_name);

std::string WorkerName(std::string_view lock_name);

}  // namespace dist_lock::impl

USERVER_NAMESPACE_END
