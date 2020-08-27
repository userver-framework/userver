#pragma once

#include <chrono>

#include <storages/redis/impl/base.hpp>
#include <storages/redis/impl/types.hpp>

namespace redis {

class RequestFuture {
  friend class Request;

 public:
  RequestFuture() = default;
  RequestFuture(RequestFuture&& r)
      : ro_future_(std::move(r.ro_future_)), until_(std::move(r.until_)) {}
  RequestFuture(ReplyPtrFuture&& fut, std::chrono::steady_clock::time_point&& t)
      : ro_future_(std::move(fut)), until_(std::move(t)) {}

  ~RequestFuture() = default;

  RequestFuture& operator=(RequestFuture&& r) {
    ro_future_ = std::move(r.ro_future_);
    until_ = std::move(r.until_);
    return *this;
  }

  RequestFuture(const RequestFuture&) = delete;
  RequestFuture& operator=(const RequestFuture&) = delete;

  ReplyPtr Get();

 private:
  ReplyPtrFuture ro_future_;
  std::chrono::steady_clock::time_point until_;
};

class Request {
 public:
  Request(Request&& r) : request_future_(std::move(r.request_future_)) {}

  Request& operator=(Request&& r) {
    request_future_ = std::move(r.request_future_);
    return *this;
  }

  Request(const Request&) = delete;
  Request& operator=(const Request&) = delete;

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
