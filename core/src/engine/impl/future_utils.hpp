#pragma once

#include <engine/task/task_context.hpp>
#include <userver/engine/future_status.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::impl {

inline FutureStatus ToFutureStatus(TaskContext::WakeupSource wakeup_source) noexcept {
    switch (wakeup_source) {
        case impl::TaskContext::WakeupSource::kNotify:
            return FutureStatus::kReady;
        case impl::TaskContext::WakeupSource::kDeadlineTimer:
            return FutureStatus::kTimeout;
        case impl::TaskContext::WakeupSource::kCancelRequest:
            return FutureStatus::kCancelled;
        case impl::TaskContext::WakeupSource::kBootstrap:
        case impl::TaskContext::WakeupSource::kNone:
            break;
    }
    utils::AbortWithStacktrace("Unexpected wakeup source");
}

}  // namespace engine::impl

USERVER_NAMESPACE_END
