#include <userver/storages/redis/hexpiretime_reply.hpp>

#include <userver/storages/redis/reply.hpp>
#include <userver/storages/redis/ttl_reply.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::redis {

HexpiretimeReply::HexpiretimeReply(int64_t value)
    : value_(value)
{}

HexpiretimeReply::HexpiretimeReply(Status status)
    : value_(static_cast<int64_t>(status))
{}

HexpiretimeReply HexpiretimeReply::Parse(ReplyData&& reply_data, const std::string& request_description) {
    reply_data.ExpectInt(request_description);
    return HexpiretimeReply(reply_data.GetInt());
}

bool HexpiretimeReply::FieldExists() const { return value_ != static_cast<int64_t>(kFieldDoesNotExist); }

bool HexpiretimeReply::HasExpiration() const { return value_ >= 0; }

std::chrono::system_clock::time_point HexpiretimeReply::GetDeadline() const {
    if (!HasExpiration()) {
        throw KeyHasNoExpirationException("hash field has no associated expire or does not exist");
    }
    return std::chrono::system_clock::time_point{std::chrono::seconds{value_}};
}

}  // namespace storages::redis

USERVER_NAMESPACE_END
