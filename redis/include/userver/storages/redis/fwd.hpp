#pragma once

#include <userver/storages/redis/client_fwd.hpp>

#include <memory>

USERVER_NAMESPACE_BEGIN

namespace storages::redis {

class Reply;
class ReplyData;

struct CommandControl;

template <typename Result, typename ReplyType = Result>
class [[nodiscard]] Request;

using ReplyPtr = std::shared_ptr<Reply>;

}  // namespace storages::redis

USERVER_NAMESPACE_END
