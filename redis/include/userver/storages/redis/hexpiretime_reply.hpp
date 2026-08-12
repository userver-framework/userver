#pragma once

/// @file userver/storages/redis/hexpiretime_reply.hpp
/// @brief Typed reply for HEXPIRETIME (per-hash-field absolute expiration
/// timestamp, seconds precision).

#include <chrono>
#include <cstdint>
#include <string>

#include <userver/storages/redis/fwd.hpp>
#include <userver/storages/redis/ttl_reply.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::redis {

/// @brief Parsed HEXPIRETIME reply for a single field — absolute deadline (in
/// seconds since the epoch) or a "field is missing" / "field has no
/// expiration" sentinel.
///
/// For millisecond-precision command (HPEXPIRETIME) use @c HpexpiretimeReply.
class HexpiretimeReply final {
public:
    enum class Status : std::int8_t {
        kFieldDoesNotExist = -2,
        kFieldHasNoExpiration = -1,
    };

    static constexpr Status kFieldDoesNotExist = Status::kFieldDoesNotExist;
    static constexpr Status kFieldHasNoExpiration = Status::kFieldHasNoExpiration;

    explicit HexpiretimeReply(int64_t value);
    HexpiretimeReply(Status status);

    /// @brief Parse a single HEXPIRETIME element from the array reply.
    static HexpiretimeReply Parse(ReplyData&& reply_data, const std::string& request_description = {});

    bool FieldExists() const;
    bool HasExpiration() const;

    /// @brief Returns the absolute deadline as a @c system_clock::time_point.
    /// Throws @c KeyHasNoExpirationException if the field has no expiration.
    std::chrono::system_clock::time_point GetDeadline() const;

private:
    int64_t value_;
};

}  // namespace storages::redis

USERVER_NAMESPACE_END
