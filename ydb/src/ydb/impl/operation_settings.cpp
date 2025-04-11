#include <ydb/impl/operation_settings.hpp>

#include <algorithm>

#include <userver/ydb/exceptions.hpp>

USERVER_NAMESPACE_BEGIN

namespace ydb::impl {

namespace {

std::chrono::milliseconds DeadlineToTimeout(engine::Deadline deadline) {
    const auto timeout = std::chrono::duration_cast<std::chrono::milliseconds>(
        deadline.IsReachable() ? deadline.TimeLeft() : engine::Deadline::Duration::max()
    );
    if (timeout <= std::chrono::milliseconds::zero()) {
        throw DeadlineExceededError("deadline exceeded before the query");
    }
    return timeout;
}

}  // namespace

std::chrono::milliseconds GetBoundTimeout(std::chrono::milliseconds timeout, engine::Deadline deadline) {
    const auto max_timeout = impl::DeadlineToTimeout(deadline);
    return (std::chrono::milliseconds::zero() < timeout) ? std::min(timeout, max_timeout) : max_timeout;
}

}  // namespace ydb::impl

USERVER_NAMESPACE_END
