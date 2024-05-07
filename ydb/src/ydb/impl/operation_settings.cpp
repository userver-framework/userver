#include <ydb/impl/operation_settings.hpp>

#include <userver/ydb/exceptions.hpp>

USERVER_NAMESPACE_BEGIN

namespace ydb::impl {

std::chrono::milliseconds DeadlineToTimeout(engine::Deadline deadline) {
  const auto timeout = std::chrono::duration_cast<std::chrono::milliseconds>(
      deadline.IsReachable() ? deadline.TimeLeft()
                             : engine::Deadline::Duration::max());
  if (timeout <= std::chrono::milliseconds::zero()) {
    // Cannot add tag to span because we're in YDB's thread
    throw DeadlineExceededError("deadline exceeded before the query");
  }
  return timeout;
}

}  // namespace ydb::impl

USERVER_NAMESPACE_END
