#pragma once

#include <chrono>
#include <cstdint>
#include <string>

#include <userver/storages/redis/exception.hpp>
#include <userver/storages/redis/fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::redis {

class TtlReply final {
public:
    enum class TtlReplyValue { kKeyDoesNotExist = -2, kKeyHasNoExpiration = -1 };

    static constexpr TtlReplyValue kKeyDoesNotExist = TtlReplyValue::kKeyDoesNotExist;
    static constexpr TtlReplyValue kKeyHasNoExpiration = TtlReplyValue::kKeyHasNoExpiration;

    explicit TtlReply(int64_t value);
    TtlReply(TtlReplyValue value);

    static TtlReply Parse(ReplyData&& reply_data, const std::string& request_description = {});

    bool KeyExists() const;
    bool KeyHasExpiration() const;
    [[deprecated("Use GetExpire() instead")]] size_t GetExpireSeconds() const { return GetExpire().count(); }
    std::chrono::seconds GetExpire() const;

private:
    int64_t value_;
};

/// Trying to get expiration from a nonexistent or a persistent key
class KeyHasNoExpirationException : public Exception {
public:
    using Exception::Exception;
};

}  // namespace storages::redis

USERVER_NAMESPACE_END
