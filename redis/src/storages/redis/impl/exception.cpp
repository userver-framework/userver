#include <userver/storages/redis/impl/exception.hpp>

#include <fmt/format.h>

#include <storages/redis/impl/reply_status_strings.hpp>

USERVER_NAMESPACE_BEGIN

namespace redis {

RequestFailedException::RequestFailedException(
    const std::string& request_description, ReplyStatus status)
    : Exception(fmt::format("{} request failed with status '{}'",
                            request_description,
                            *kReplyStatusMap.TryFind(status))),
      status_(status) {}

ReplyStatus RequestFailedException::GetStatus() const { return status_; }

std::string_view RequestFailedException::GetStatusString() const {
  return *kReplyStatusMap.TryFind(status_);
}

bool RequestFailedException::IsTimeout() const {
  return status_ == ReplyStatus::kTimeoutError;
}

}  // namespace redis

USERVER_NAMESPACE_END
