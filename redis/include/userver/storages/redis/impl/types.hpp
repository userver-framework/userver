#pragma once

#include <memory>

USERVER_NAMESPACE_BEGIN

namespace redis {

class Reply;
class ReplyData;
class Sentinel;
class SubscribeSentinel;
// NOLINTNEXTLINE(bugprone-forward-declaration-namespace)
class Request;
struct Command;

using ReplyPtr = std::shared_ptr<Reply>;
using CommandPtr = std::shared_ptr<Command>;

}  // namespace redis

USERVER_NAMESPACE_END
