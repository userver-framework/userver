#pragma once

#include <memory>

USERVER_NAMESPACE_BEGIN

namespace redis {
class ReplyData;
class Reply;
}  // namespace redis

namespace storages {
namespace redis {

using ReplyData = USERVER_NAMESPACE::redis::ReplyData;
using Reply = USERVER_NAMESPACE::redis::Reply;

using ReplyPtr = std::shared_ptr<Reply>;

}  // namespace redis
}  // namespace storages

USERVER_NAMESPACE_END
