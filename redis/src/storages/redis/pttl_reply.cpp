#include <userver/storages/redis/pttl_reply.hpp>

#include <userver/storages/redis/reply.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::redis {

PttlReply::PttlReply(int64_t value)
    : value_(value)
{}

PttlReply::PttlReply(PttlReplyValue value)
    : value_(static_cast<int64_t>(value))
{}

PttlReply PttlReply::Parse(ReplyData&& reply_data, const std::string& request_description) {
    reply_data.ExpectInt(request_description);
    return PttlReply(reply_data.GetInt());
}

bool PttlReply::KeyExists() const { return value_ != static_cast<int64_t>(kKeyDoesNotExist); }

bool PttlReply::KeyHasExpiration() const { return value_ >= 0; }

std::chrono::milliseconds PttlReply::GetExpire() const {
    if (!KeyHasExpiration()) {
        throw KeyHasNoExpirationException("key has no associated expire or does not exist");
    }
    return std::chrono::milliseconds{value_};
}

}  // namespace storages::redis

USERVER_NAMESPACE_END
