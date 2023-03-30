#pragma once

USERVER_NAMESPACE_BEGIN

namespace utils {

bool IsMainThread();

void SetCurrentThreadIdleScheduling();

void SetCurrentThreadLowPriorityScheduling();

}  // namespace utils

USERVER_NAMESPACE_END
