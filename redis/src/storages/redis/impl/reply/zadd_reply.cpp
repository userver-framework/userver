#include "zadd_reply.hpp"

#include <userver/storages/redis/impl/exception.hpp>
#include <userver/storages/redis/impl/reply.hpp>

USERVER_NAMESPACE_BEGIN

namespace redis {

ZaddReply::ZaddReply(size_t value) : value_(value) {}

ZaddReply ZaddReply::Parse(ReplyData&& reply_data,
                           const std::string& request_description) {
  reply_data.ExpectInt(request_description);

  int64_t value = reply_data.GetInt();
  if (value < 0) {
    throw ParseReplyException("Unexpected negative ZADD reply value: " +
                              reply_data.ToDebugString());
  }
  return ZaddReply(value);
}

size_t ZaddReply::GetCount() const { return value_; }

}  // namespace redis

USERVER_NAMESPACE_END
