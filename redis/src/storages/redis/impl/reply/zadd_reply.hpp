#pragma once

#include <userver/storages/redis/impl/types.hpp>

namespace redis {

class ZaddReply final {
 public:
  explicit ZaddReply(size_t value);

  static ZaddReply Parse(ReplyData&& reply_data,
                         const std::string& request_description = {});

  size_t GetCount() const;

 private:
  size_t value_;
};

}  // namespace redis
