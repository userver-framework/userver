#pragma once

#include <chrono>

#include <userver/engine/deadline.hpp>
#include <userver/engine/future.hpp>

#include <userver/storages/redis/impl/base.hpp>
#include <userver/storages/redis/impl/types.hpp>

USERVER_NAMESPACE_BEGIN

namespace tracing {
class Span;
}

namespace redis {

class Request {
 public:
  Request(const Request&) = delete;
  Request(Request&& r) noexcept = default;
  Request& operator=(const Request&) = delete;
  Request& operator=(Request&& r) noexcept = default;

  ReplyPtr Get();

 private:
  friend class Sentinel;

  Request(Sentinel& sentinel, CmdArgs&& args, const std::string& key,
          bool master, const CommandControl& command_control,
          size_t replies_to_skip);

  Request(Sentinel& sentinel, CmdArgs&& args, size_t shard, bool master,
          const CommandControl& command_control, size_t replies_to_skip);

  CommandPtr PrepareRequest(CmdArgs&& args,
                            const CommandControl& command_control,
                            size_t replies_to_skip);

  engine::Future<ReplyPtr> future_;
  engine::Deadline deadline_;
};

}  // namespace redis

USERVER_NAMESPACE_END
