#pragma once

#include <chrono>
#include <functional>
#include <future>
#include <memory>

#include <userver/engine/future.hpp>

USERVER_NAMESPACE_BEGIN

namespace redis {

class Reply;
class ReplyData;
class Sentinel;
class SubscribeSentinel;
class Request;
struct Command;

using ReplyPtr = std::shared_ptr<Reply>;
using CommandPtr = std::shared_ptr<Command>;

using ReplyCallback =
    std::function<void(const CommandPtr& cmd, ReplyPtr reply)>;

using ReplyPtrPromise = engine::Promise<ReplyPtr>;
using ReplyPtrFuture = engine::Future<ReplyPtr>;

using ReplyCallbackEx = std::function<void(const CommandPtr& cmd,
                                           ReplyPtr reply, ReplyPtrPromise&)>;

}  // namespace redis

USERVER_NAMESPACE_END
