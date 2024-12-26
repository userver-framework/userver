#include <userver/storages/redis/expire_reply.hpp>

#include <userver/storages/redis/exception.hpp>
#include <userver/storages/redis/reply.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::redis {

ExpireReply::ExpireReply(int64_t value) {
    switch (value) {
        case 0:
            value_ = kKeyDoesNotExist;
            break;
        case 1:
            value_ = kTimeoutWasSet;
            break;
        default:
            throw ParseReplyException("Incorrect EXPIRE result value: " + std::to_string(value));
    }
}

ExpireReply::ExpireReply(ExpireReplyValue value) : value_(value) {}

ExpireReply ExpireReply::Parse(ReplyData&& reply_data, const std::string& request_description) {
    reply_data.ExpectInt(request_description);
    return ExpireReply(reply_data.GetInt());
}

ExpireReply::operator ExpireReplyValue() const { return value_; }

}  // namespace storages::redis

USERVER_NAMESPACE_END
