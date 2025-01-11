#pragma once

#include <memory>

USERVER_NAMESPACE_BEGIN

namespace storages::redis {

class Reply;
class ReplyData;

using ReplyPtr = std::shared_ptr<Reply>;

}  // namespace storages::redis

USERVER_NAMESPACE_END
