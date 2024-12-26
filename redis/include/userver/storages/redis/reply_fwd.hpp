#pragma once

#include <memory>

USERVER_NAMESPACE_BEGIN

namespace storages::redis {

class Reply;
class ReplyData;

using ReplyPtr = std::shared_ptr<Reply>;

}  // namespace storages::redis

#ifdef USERVER_FEATURE_LEGACY_REDIS_NAMESPACE
namespace redis {
using storages::redis::Reply;
using storages::redis::ReplyData;
using storages::redis::ReplyPtr;
}  // namespace redis
#endif

USERVER_NAMESPACE_END
