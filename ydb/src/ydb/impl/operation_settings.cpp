#include <ydb/impl/operation_settings.hpp>

#include <algorithm>

#include <userver/ydb/exceptions.hpp>

using namespace std::chrono_literals;

USERVER_NAMESPACE_BEGIN

namespace ydb::impl {

namespace {

std::chrono::milliseconds DeadlineToTimeout(engine::Deadline deadline) {
    const auto timeout = std::chrono::duration_cast<
        std::chrono::milliseconds>(deadline.IsReachable() ? deadline.TimeLeft() : engine::Deadline::Duration::max());
    if (timeout <= 0ms) {
        throw DeadlineExceededError("deadline exceeded before the query");
    }
    return timeout;
}

}  // namespace

std::chrono::milliseconds GetBoundTimeout(std::chrono::milliseconds timeout, engine::Deadline deadline) {
    const auto max_timeout = impl::DeadlineToTimeout(deadline);
    return (0ms < timeout) ? std::min(timeout, max_timeout) : max_timeout;
}

std::chrono::milliseconds GetBoundTimeout(std::optional<std::chrono::milliseconds> timeout, engine::Deadline deadline) {
    const auto max_timeout = impl::DeadlineToTimeout(deadline);
    return timeout.has_value() ? GetBoundTimeout(*timeout, deadline) : max_timeout;
}

}  // namespace ydb::impl

USERVER_NAMESPACE_END
