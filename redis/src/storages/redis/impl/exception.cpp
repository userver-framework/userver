#include <userver/storages/redis/impl/exception.hpp>

#include <fmt/format.h>

#include <userver/storages/redis/impl/base.hpp>
#include <userver/storages/redis/impl/reply.hpp>

namespace redis {

RequestFailedException::RequestFailedException(
    const std::string& request_description, int status)
    : Exception(fmt::format("{} request failed with status {} ({})",
                            request_description, status,
                            Reply::StatusToString(status))),
      status_(status) {}

int RequestFailedException::GetStatus() const { return status_; }

const std::string& RequestFailedException::GetStatusString() const {
  return Reply::StatusToString(status_);
}

bool RequestFailedException::IsTimeout() const {
  return status_ == REDIS_ERR_TIMEOUT;
}

}  // namespace redis
