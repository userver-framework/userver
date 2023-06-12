#pragma once

#include <cstdint>
#include <string>

#include <userver/storages/redis/impl/types.hpp>

USERVER_NAMESPACE_BEGIN

namespace redis {

class ExpireReply final {
 public:
  enum class ExpireReplyValue { kKeyDoesNotExist, kTimeoutWasSet };

  static constexpr ExpireReplyValue kKeyDoesNotExist =
      ExpireReplyValue::kKeyDoesNotExist;
  static constexpr ExpireReplyValue kTimeoutWasSet =
      ExpireReplyValue::kTimeoutWasSet;

  explicit ExpireReply(int64_t value);
  ExpireReply(ExpireReplyValue value);

  static ExpireReply Parse(ReplyData&& reply_data,
                           const std::string& request_description = {});

  operator ExpireReplyValue() const;

 private:
  ExpireReplyValue value_;
};

}  // namespace redis

USERVER_NAMESPACE_END
