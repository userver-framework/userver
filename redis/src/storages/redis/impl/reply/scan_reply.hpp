#pragma once

#include <userver/storages/redis/impl/reply.hpp>

USERVER_NAMESPACE_BEGIN

namespace redis {

struct ScanReply {
  std::optional<ScanCursor> cursor;
  std::vector<std::string> keys;

  static ScanReply parse(ReplyPtr reply);
};

}  // namespace redis

USERVER_NAMESPACE_END
