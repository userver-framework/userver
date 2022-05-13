#pragma once

#include <condition_variable>
#include <mutex>
#include <type_traits>
#include <utility>

#include <engine/ev/async_payload_base.hpp>
#include <engine/ev/thread.hpp>
#include <userver/engine/single_use_event.hpp>
#include <userver/utils/fast_scope_guard.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::ev {

namespace impl {

template <typename Func>
class UniquePayloadAsync final : public AsyncPayloadBase {
  static_assert(!std::is_reference_v<Func>);

 public:
  explicit UniquePayloadAsync(Func&& func)
      : AsyncPayloadBase(&Release), func_(std::move(func)) {}

  explicit UniquePayloadAsync(const Func& func)
      : AsyncPayloadBase(&Release), func_(func) {}

  void Invoke() { func_(); }

 private:
  static void Release(AsyncPayloadBase& data) noexcept {
    delete &static_cast<UniquePayloadAsync&>(data);
  }

  Func func_;
};

template <typename Func>
class CallerOwnedPayloadSync final : public AsyncPayloadBase {
  static_assert(!std::is_reference_v<Func>);

 public:
  explicit CallerOwnedPayloadSync(Func& func)
      : AsyncPayloadBase(&Noop), func_(func) {}

  void Invoke() {
    utils::FastScopeGuard guard([this]() noexcept { event_.Send(); });
    func_();
  }

  void Wait() noexcept { event_.WaitNonCancellable(); }

 private:
  Func& func_;
  engine::SingleUseEvent event_;
};

template <typename Func>
class CallerOwnedPayloadBlocking final : public AsyncPayloadBase {
  static_assert(!std::is_reference_v<Func>);

 public:
  explicit CallerOwnedPayloadBlocking(Func& func) noexcept
      : AsyncPayloadBase(&Noop), func_(func) {}

  void Invoke() {
    utils::FastScopeGuard guard([this]() noexcept { Notify(); });
    func_();
  }

  void Wait() noexcept {
    std::unique_lock lock(mutex_);
    cv_.wait(lock, [&] { return finished_; });
  }

 private:
  void Notify() noexcept {
    {
      std::lock_guard lock(mutex_);
      finished_ = true;
    }
    cv_.notify_one();
  }

  Func& func_;
  std::mutex mutex_;
  std::condition_variable cv_;
  bool finished_{false};
};

}  // namespace impl

class ThreadControl final {
 public:
  explicit ThreadControl(Thread& thread) noexcept : thread_(thread) {}

  struct ev_loop* GetEvLoop() const noexcept;

  void AsyncStartUnsafe(ev_async& w) { thread_.AsyncStartUnsafe(w); }

  void AsyncStart(ev_async& w) { thread_.AsyncStart(w); }

  void AsyncStopUnsafe(ev_async& w) { thread_.AsyncStopUnsafe(w); }

  void AsyncStop(ev_async& w) { thread_.AsyncStop(w); }

  void TimerStartUnsafe(ev_timer& w) { thread_.TimerStartUnsafe(w); }

  void TimerStart(ev_timer& w) { thread_.TimerStart(w); }

  void TimerAgainUnsafe(ev_timer& w) { thread_.TimerAgainUnsafe(w); }

  void TimerAgain(ev_timer& w) { thread_.TimerAgain(w); }

  void TimerStopUnsafe(ev_timer& w) { thread_.TimerStopUnsafe(w); }

  void TimerStop(ev_timer& w) { thread_.TimerStop(w); }

  void IoStartUnsafe(ev_io& w) { thread_.IoStartUnsafe(w); }

  void IoStart(ev_io& w) { thread_.IoStart(w); }

  void IoStopUnsafe(ev_io& w) { thread_.IoStopUnsafe(w); }

  void IoStop(ev_io& w) { thread_.IoStop(w); }

  void IdleStartUnsafe(ev_idle& w) { thread_.IdleStartUnsafe(w); }

  void IdleStart(ev_idle& w) { thread_.IdleStart(w); }

  void IdleStopUnsafe(ev_idle& w) { thread_.IdleStopUnsafe(w); }

  void IdleStop(ev_idle& w) { thread_.IdleStop(w); }

  /// Fast non allocating function to execute a `func(*data)` in EvLoop
  void RunInEvLoopAsync(OnAsyncPayload* func, AsyncPayloadPtr&& data);

  /// Allocating function to execute `func()` in EvLoop.
  ///
  /// Dynamically allocates storage and forwards func into it, passes storage
  /// and helper lambda into the two argument RunInEvLoopAsync overload.
  template <typename Func>
  void RunInEvLoopAsync(Func&& func);

  /// Fast non allocating function to register a `func(*data)` in EvLoop.
  /// Depending on thread settings might fallback to RunInEvLoopAsync.
  void RunInEvLoopDeferred(OnAsyncPayload* func, AsyncPayloadPtr&& data,
                           Deadline deadline);

  /// Allocating function to execute `func()` in EvLoop.
  ///
  /// Dynamically allocates storage and forwards func into it, passes
  /// storage and helper lambda into the three argument RunInEvLoopDeferred
  /// overload, with deadline being unreachable.
  template <typename Func>
  void RunInEvLoopDeferred(Func&& func);

  template <typename Func>
  void RunInEvLoopSync(Func&& func);

  template <typename Func>
  void RunInEvLoopBlocking(Func&& func);

  bool IsInEvThread() const noexcept;

 private:
  Thread& thread_;
};

template <typename Func>
void ThreadControl::RunInEvLoopAsync(Func&& func) {
  if (IsInEvThread()) {
    func();
    return;
  }

  using Payload = impl::UniquePayloadAsync<std::remove_reference_t<Func>>;
  AsyncPayloadPtr data{new Payload(std::forward<Func>(func))};

  RunInEvLoopAsync(
      [](AsyncPayloadPtr&& ptr) { static_cast<Payload&>(*ptr).Invoke(); },
      std::move(data));
}

template <typename Func>
void ThreadControl::RunInEvLoopDeferred(Func&& func) {
  if (IsInEvThread()) {
    func();
    return;
  }

  using Payload = impl::UniquePayloadAsync<std::remove_reference_t<Func>>;
  AsyncPayloadPtr data{new Payload(std::forward<Func>(func))};

  RunInEvLoopDeferred(
      [](AsyncPayloadPtr&& ptr) { static_cast<Payload&>(*ptr).Invoke(); },
      std::move(data), {});
}

template <typename Func>
void ThreadControl::RunInEvLoopSync(Func&& func) {
  if (IsInEvThread()) {
    func();
    return;
  }

  using Payload = impl::CallerOwnedPayloadSync<std::remove_reference_t<Func>>;
  Payload data{func};

  RunInEvLoopAsync(
      [](AsyncPayloadPtr&& ptr) {
        // 'release' is necessary, because Payload may be destroyed after
        // signalling.
        static_cast<Payload&>(*ptr.release()).Invoke();
      },
      AsyncPayloadPtr(&data));

  data.Wait();
}

template <typename Func>
void ThreadControl::RunInEvLoopBlocking(Func&& func) {
  if (IsInEvThread()) {
    func();
    return;
  }

  using Payload =
      impl::CallerOwnedPayloadBlocking<std::remove_reference_t<Func>>;
  Payload data{func};

  RunInEvLoopAsync(
      [](AsyncPayloadPtr&& ptr) {
        // 'release' is necessary, because Payload may be destroyed after
        // signalling.
        static_cast<Payload&>(*ptr.release()).Invoke();
      },
      AsyncPayloadPtr(&data));

  data.Wait();
}

}  // namespace engine::ev

USERVER_NAMESPACE_END
