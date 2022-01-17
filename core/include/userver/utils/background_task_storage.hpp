#pragma once

#include <userver/concurrent/background_task_storage.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils {

// TODO remove in TAXICOMMON-4722
using BackgroundTasksStorage = concurrent::BackgroundTaskStorage;

}  // namespace utils

USERVER_NAMESPACE_END
