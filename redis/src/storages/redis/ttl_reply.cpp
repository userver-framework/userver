#include <userver/storages/redis/ttl_reply.hpp>

#include <userver/storages/redis/reply.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::redis {

TtlReply::TtlReply(int64_t value) : value_(value) {}

TtlReply::TtlReply(TtlReplyValue value) : value_(static_cast<int64_t>(value)) {}

TtlReply TtlReply::Parse(ReplyData&& reply_data, const std::string& request_description) {
    reply_data.ExpectInt(request_description);
    return TtlReply(reply_data.GetInt());
}

bool TtlReply::KeyExists() const { return value_ != static_cast<int64_t>(kKeyDoesNotExist); }

bool TtlReply::KeyHasExpiration() const { return value_ >= 0; }

size_t TtlReply::GetExpireSeconds() const {
    if (!KeyHasExpiration()) throw KeyHasNoExpirationException("key has no associated expire or does not exist");
    return value_;
}

}  // namespace storages::redis

USERVER_NAMESPACE_END
