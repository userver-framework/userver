#include "zadd_reply.hpp"

#include <userver/storages/redis/exception.hpp>
#include <userver/storages/redis/reply.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::redis::impl {

ZaddReply::ZaddReply(size_t value) : value_(value) {}

ZaddReply ZaddReply::Parse(ReplyData&& reply_data, const std::string& request_description) {
    reply_data.ExpectInt(request_description);

    const int64_t value = reply_data.GetInt();
    if (value < 0) {
        throw ParseReplyException("Unexpected negative ZADD reply value: " + reply_data.ToDebugString());
    }
    return ZaddReply(value);
}

size_t ZaddReply::GetCount() const { return value_; }

}  // namespace storages::redis::impl

USERVER_NAMESPACE_END
