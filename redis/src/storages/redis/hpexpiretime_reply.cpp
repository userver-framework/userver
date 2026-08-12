#include <userver/storages/redis/hpexpiretime_reply.hpp>

#include <userver/storages/redis/reply.hpp>
#include <userver/storages/redis/ttl_reply.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::redis {

HpexpiretimeReply::HpexpiretimeReply(int64_t value)
    : value_(value)
{}

HpexpiretimeReply::HpexpiretimeReply(Status status)
    : value_(static_cast<int64_t>(status))
{}

HpexpiretimeReply HpexpiretimeReply::Parse(ReplyData&& reply_data, const std::string& request_description) {
    reply_data.ExpectInt(request_description);
    return HpexpiretimeReply(reply_data.GetInt());
}

bool HpexpiretimeReply::FieldExists() const { return value_ != static_cast<int64_t>(kFieldDoesNotExist); }

bool HpexpiretimeReply::HasExpiration() const { return value_ >= 0; }

std::chrono::system_clock::time_point HpexpiretimeReply::GetDeadline() const {
    if (!HasExpiration()) {
        throw KeyHasNoExpirationException("hash field has no associated expire or does not exist");
    }
    return std::chrono::system_clock::time_point{std::chrono::milliseconds{value_}};
}

}  // namespace storages::redis

USERVER_NAMESPACE_END
