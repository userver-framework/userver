#pragma once

/// @file userver/storages/redis/hpexpiretime_reply.hpp
/// @brief Typed reply for HPEXPIRETIME (per-hash-field absolute expiration
/// timestamp, millisecond precision).

#include <chrono>
#include <cstdint>
#include <string>

#include <userver/storages/redis/fwd.hpp>
#include <userver/storages/redis/ttl_reply.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::redis {

/// @brief Parsed HPEXPIRETIME reply for a single field — absolute deadline (in
/// milliseconds since the epoch) or a "field is missing" / "field has no
/// expiration" sentinel.
///
/// Sibling of @c HexpiretimeReply for the milliseconds-precision command.
class HpexpiretimeReply final {
public:
    enum class Status : std::int8_t {
        kFieldDoesNotExist = -2,
        kFieldHasNoExpiration = -1,
    };

    static constexpr Status kFieldDoesNotExist = Status::kFieldDoesNotExist;
    static constexpr Status kFieldHasNoExpiration = Status::kFieldHasNoExpiration;

    explicit HpexpiretimeReply(int64_t value);
    HpexpiretimeReply(Status status);

    /// @brief Parse a single HPEXPIRETIME element from the array reply.
    static HpexpiretimeReply Parse(ReplyData&& reply_data, const std::string& request_description = {});

    bool FieldExists() const;
    bool HasExpiration() const;

    /// @brief Returns the absolute deadline as a @c system_clock::time_point
    /// (with millisecond-precision underlying value). Throws
    /// @c KeyHasNoExpirationException if the field has no expiration.
    std::chrono::system_clock::time_point GetDeadline() const;

private:
    int64_t value_;
};

}  // namespace storages::redis

USERVER_NAMESPACE_END
