#pragma once

#include <chrono>
#include <string>

#include <engine/future.hpp>

#include "base.hpp"
#include "command.hpp"
#include "reply.hpp"

namespace storages {
namespace redis {

class Sentinel;

class Request {
 public:
  Request(Request&& r)
      : ro_future_(std::move(r.ro_future_)), until_(std::move(r.until_)) {}

  Request(Request& r) = delete;
  Request& operator=(Request&) = delete;
  Request& operator=(Request&&) = delete;

  ReplyPtr Get();

  friend class Sentinel;

 private:
  Request(Sentinel& sentinel, CmdArgs&& args, const std::string& key,
          bool master, const CommandControl& command_control,
          bool skip_status = false);

  Request(Sentinel& sentinel, CmdArgs&& args, size_t shard, bool master,
          const CommandControl& command_control, bool skip_status = false);

  CommandPtr PrepareRequest(CmdArgs&& args,
                            const CommandControl& command_control,
                            bool skip_status);

  engine::Future<ReplyPtr> ro_future_;
  std::chrono::steady_clock::time_point until_;
};

}  // namespace redis
}  // namespace storages
