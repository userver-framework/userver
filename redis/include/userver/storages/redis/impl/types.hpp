#pragma once

#include <chrono>
#include <functional>
#include <future>
#include <memory>

#include <utils/fast_pimpl.hpp>

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

class ReplyPtrFutureImpl;
class ReplyPtrFuture {
 public:
  ReplyPtrFuture();
  explicit ReplyPtrFuture(ReplyPtrFutureImpl&&);
  ~ReplyPtrFuture();

  ReplyPtrFuture(const ReplyPtrFuture&) = delete;
  ReplyPtrFuture(ReplyPtrFuture&&) noexcept;
  ReplyPtrFuture& operator=(const ReplyPtrFuture&) = delete;
  ReplyPtrFuture& operator=(ReplyPtrFuture&&) noexcept;

  std::future_status wait_until(
      const std::chrono::steady_clock::time_point&) const;
  ReplyPtr get();

 private:
  static constexpr size_t kImplSize = 16;
  static constexpr size_t kImplAlignment = 8;
  utils::FastPimpl<ReplyPtrFutureImpl, kImplSize, kImplAlignment> impl_;
};

class ReplyPtrPromiseImpl;
class ReplyPtrPromise {
 public:
  ReplyPtrPromise();
  ~ReplyPtrPromise();

  ReplyPtrPromise(const ReplyPtrPromise&) = delete;
  ReplyPtrPromise(ReplyPtrPromise&&) noexcept;
  ReplyPtrPromise& operator=(const ReplyPtrPromise&) = delete;
  ReplyPtrPromise& operator=(ReplyPtrPromise&&) noexcept;

  ReplyPtrFuture get_future();
  void set_value(ReplyPtr);

 private:
  static constexpr size_t kImplSize = 16;
  static constexpr size_t kImplAlignment = 8;
  utils::FastPimpl<ReplyPtrPromiseImpl, kImplSize, kImplAlignment> impl_;
};

using ReplyCallbackEx = std::function<void(const CommandPtr& cmd,
                                           ReplyPtr reply, ReplyPtrPromise&)>;

}  // namespace redis
