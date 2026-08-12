#pragma once

#include <userver/storages/redis/reply.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::redis::impl {

struct ScanReply {
    std::optional<ScanCursor> cursor;
    std::vector<std::string> keys;

    static ScanReply parse(ReplyPtr reply);
};

}  // namespace storages::redis::impl

USERVER_NAMESPACE_END
