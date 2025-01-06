#pragma once

#include <cstdint>
#include <string>

#include <userver/storages/redis/fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::redis {

class ExpireReply final {
public:
    enum class ExpireReplyValue { kKeyDoesNotExist, kTimeoutWasSet };

    static constexpr ExpireReplyValue kKeyDoesNotExist = ExpireReplyValue::kKeyDoesNotExist;
    static constexpr ExpireReplyValue kTimeoutWasSet = ExpireReplyValue::kTimeoutWasSet;

    explicit ExpireReply(int64_t value);
    ExpireReply(ExpireReplyValue value);

    static ExpireReply Parse(ReplyData&& reply_data, const std::string& request_description = {});

    operator ExpireReplyValue() const;

private:
    ExpireReplyValue value_;
};

}  // namespace storages::redis

#ifdef USERVER_FEATURE_LEGACY_REDIS_NAMESPACE
namespace redis {
using storages::redis::ExpireReply;
}
#endif

USERVER_NAMESPACE_END
