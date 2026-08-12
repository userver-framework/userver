#pragma once

/// @file userver/storages/redis/pttl_reply.hpp
/// @brief Redis PTTL / HPTTL reply value (millisecond precision).

#include <chrono>
#include <cstdint>
#include <string>

#include <userver/storages/redis/fwd.hpp>
#include <userver/storages/redis/ttl_reply.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::redis {

/// @brief Parsed PTTL / HPTTL reply (millisecond precision).
///
/// Sibling of @c TtlReply for the millisecond-precision variants of the TTL
/// commands. Reuses @c KeyHasNoExpirationException for its error case.
class PttlReply final {
public:
    enum class PttlReplyValue : std::int8_t {
        kKeyDoesNotExist = -2,
        kKeyHasNoExpiration = -1,
    };

    static constexpr PttlReplyValue kKeyDoesNotExist = PttlReplyValue::kKeyDoesNotExist;
    static constexpr PttlReplyValue kKeyHasNoExpiration = PttlReplyValue::kKeyHasNoExpiration;

    explicit PttlReply(int64_t value);
    PttlReply(PttlReplyValue value);

    /// @brief Parse a reply from a milliseconds-precision command (PTTL / HPTTL).
    static PttlReply Parse(ReplyData&& reply_data, const std::string& request_description = {});

    bool KeyExists() const;
    bool KeyHasExpiration() const;

    /// @brief Returns the expiration as @c std::chrono::milliseconds. Throws
    /// @c KeyHasNoExpirationException if the key/field has no expiration.
    std::chrono::milliseconds GetExpire() const;

private:
    int64_t value_;
};

}  // namespace storages::redis

USERVER_NAMESPACE_END
