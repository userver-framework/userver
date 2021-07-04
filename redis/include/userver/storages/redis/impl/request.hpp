#pragma once

#include <chrono>

#include <userver/storages/redis/impl/base.hpp>
#include <userver/storages/redis/impl/types.hpp>

namespace redis {

class RequestFuture {
  friend class Request;

 public:
  RequestFuture() = default;
  RequestFuture(ReplyPtrFuture&& fut,
                std::chrono::steady_clock::time_point t) noexcept
      : ro_future_(std::move(fut)), until_(t) {}
  ~RequestFuture() = default;

  RequestFuture(const RequestFuture&) = delete;
  RequestFuture(RequestFuture&& r) noexcept = default;
  RequestFuture& operator=(const RequestFuture&) = delete;
  RequestFuture& operator=(RequestFuture&& r) noexcept = default;

  ReplyPtr Get();

 private:
  ReplyPtrFuture ro_future_;
  std::chrono::steady_clock::time_point until_;
};

class Request {
 public:
  Request(const Request&) = delete;
  Request(Request&& r) noexcept = default;
  Request& operator=(const Request&) = delete;
  Request& operator=(Request&& r) noexcept = default;

  ReplyPtr Get();
  RequestFuture&& PopFuture();

  friend class Sentinel;

 private:
  Request(Sentinel& sentinel, CmdArgs&& args, const std::string& key,
          bool master, const CommandControl& command_control,
          size_t replies_to_skip = 0);
  Request(Sentinel& sentinel, CmdArgs&& args, size_t shard, bool master,
          const CommandControl& command_control, size_t replies_to_skip = 0);
  Request(Sentinel& sentinel, CmdArgs&& args, size_t shard, bool master,
          const CommandControl& command_control,
          redis::ReplyCallback&& callback);
  CommandPtr PrepareRequest(CmdArgs&& args,
                            const CommandControl& command_control,
                            size_t replies_to_skip = 0);

  RequestFuture request_future_;
};

}  // namespace redis
