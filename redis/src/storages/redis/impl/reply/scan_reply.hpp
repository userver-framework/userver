#pragma once

#include <storages/redis/impl/reply.hpp>

namespace redis {

struct ScanReply {
  std::optional<ScanCursor> cursor;
  std::vector<std::string> keys;

  static ScanReply parse(ReplyPtr reply);
};

}  // namespace redis
