#pragma once

#include <atomic>
#include <type_traits>
#include <utility>

#include <concurrent/impl/intrusive_hooks.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::ev {

// The base class for data for an asynchronous ev call that knows how to
// "delete" itself after the call.
class AsyncPayloadBase : public concurrent::impl::SinglyLinkedBaseHook {
 public:
  AsyncPayloadBase(AsyncPayloadBase&&) = delete;
  AsyncPayloadBase& operator=(AsyncPayloadBase&&) = delete;

  // Must be called on an ev thread.
  void PerformAndRelease() { perform_and_release_func_(*this); }

 protected:
  using PerformAndReleaseFunc = void (*)(AsyncPayloadBase&);

  explicit AsyncPayloadBase(
      PerformAndReleaseFunc perform_and_release_func) noexcept
      : perform_and_release_func_(perform_and_release_func) {
    UASSERT(perform_and_release_func_);
  }

  // Prohibit destruction via pointer to base.
  ~AsyncPayloadBase() = default;

 private:
  const PerformAndReleaseFunc perform_and_release_func_;
};

template <typename Derived>
class SingleShotAsyncPayload : public AsyncPayloadBase {
 public:
  SingleShotAsyncPayload() : AsyncPayloadBase(&PerformAndReleaseImpl) {
    static_assert(std::is_base_of_v<SingleShotAsyncPayload, Derived>);
  }

 protected:
  // Prohibit destruction via pointer to base.
  ~SingleShotAsyncPayload() = default;

 private:
  static void PerformAndReleaseImpl(AsyncPayloadBase& base) {
    auto& self = static_cast<SingleShotAsyncPayload&>(base);
#ifndef NDEBUG
    UASSERT(!self.was_performed_);
    self.was_performed_ = true;
#endif
    static_cast<Derived&>(self).DoPerformAndRelease();
    // *this may be destroyed at this point
  }

#ifndef NDEBUG
  bool was_performed_{false};
#endif
};

template <typename Derived>
class MultiShotAsyncPayload : public AsyncPayloadBase {
 public:
  MultiShotAsyncPayload() : AsyncPayloadBase(&PerformAndReleaseImpl) {
    static_assert(std::is_base_of_v<MultiShotAsyncPayload, Derived>);
  }

  // Must be called before enqueueing *this onto an ev thread.
  // Multiple enqueue operations cannot be performed concurrently.
  //
  // If returns 'false', then this payload is currently enqueued (its processing
  // hasn't started yet) and does not need to be enqueued again, because all
  // desired changes in *this will be accounted for during processing.
  //
  // A potential race condition can happen if the owner stores data into *this
  // in parallel with an already enqueued operation running on the ev thread.
  // DoPerformAndRelease should be prepared to such concurrent access, as well
  // as to double-execution with the same data.
  bool PrepareEnqueue() {
    // synchronizes-with exchange in PerformAndReleaseImpl.
    // We need make previous stores in the calling thread visible
    // to the ev thread in case is_in_queue_ was 'true'.
    // A load-store pair would introduce a race condition.
    return !is_in_queue_.exchange(true, std::memory_order_release);
  }

 private:
  static void PerformAndReleaseImpl(AsyncPayloadBase& base) {
    auto& self = static_cast<MultiShotAsyncPayload&>(base);
    // 'store' is not enough here, because we need 'acquire'.
    const bool was_in_queue =
        self.is_in_queue_.exchange(false, std::memory_order_acquire);
    UASSERT_MSG(was_in_queue,
                "Concurrent enqueues, or a forgotten PrepareEnqueue detected");
    static_cast<Derived&>(self).DoPerformAndRelease();
    // *this may be destroyed at this point
  }

  std::atomic<bool> is_in_queue_{false};
};

}  // namespace engine::ev

USERVER_NAMESPACE_END
