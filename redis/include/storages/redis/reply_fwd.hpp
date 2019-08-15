#pragma once

#include <memory>

namespace redis {
class ReplyData;
class Reply;
}  // namespace redis

namespace storages {
namespace redis {

using ReplyData = ::redis::ReplyData;
using Reply = ::redis::Reply;

using ReplyPtr = std::shared_ptr<Reply>;

}  // namespace redis
}  // namespace storages
