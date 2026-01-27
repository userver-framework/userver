#pragma once

/// @file
/// @brief Forward declarations of storages::redis::Reply, storages::redis::ReplyData, storages::redis::CommandControl,
/// storages::redis::Request, storages::redis::Client and storages::redis::SubscribeClient.

#include <userver/storages/redis/client_fwd.hpp>

#include <memory>

USERVER_NAMESPACE_BEGIN

namespace storages::redis {

class Reply;
class ReplyData;

struct CommandControl;

template <typename ResultType, typename ReplyType = ResultType>
class [[nodiscard]] Request;

using ReplyPtr = std::shared_ptr<Reply>;

}  // namespace storages::redis

USERVER_NAMESPACE_END
