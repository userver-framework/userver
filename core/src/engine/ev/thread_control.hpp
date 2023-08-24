#pragma once

#include <condition_variable>
#include <memory>
#include <mutex>
#include <type_traits>
#include <utility>

#include <ev.h>

#include <engine/ev/async_payload_base.hpp>
#include <userver/engine/deadline.hpp>
#include <userver/engine/single_use_event.hpp>
#include <userver/utils/fast_scope_guard.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine {
class Deadline;
}  // namespace engine

namespace engine::ev {

namespace impl {

template <typename Func>
class UniquePayloadAsync final
    : public SingleShotAsyncPayload<UniquePayloadAsync<Func>> {
  static_assert(!std::is_reference_v<Func>);

 public:
  explicit UniquePayloadAsync(Func&& func) : func_(std::move(func)) {}

  explicit UniquePayloadAsync(const Func& func) : func_(func) {}

  void DoPerformAndRelease() {
    utils::FastScopeGuard guard([this]() noexcept { delete this; });
    func_();
  }

 private:
  Func func_;
};

template <typename Func>
class CallerOwnedPayloadSync final
    : public SingleShotAsyncPayload<CallerOwnedPayloadSync<Func>> {
  static_assert(!std::is_reference_v<Func>);

 public:
  explicit CallerOwnedPayloadSync(Func& func) : func_(func) {}

  void DoPerformAndRelease() {
    utils::FastScopeGuard guard([this]() noexcept { event_.Send(); });
    func_();
  }

  void Wait() noexcept { event_.WaitNonCancellable(); }

 private:
  Func& func_;
  engine::SingleUseEvent event_;
};

template <typename Func>
class CallerOwnedPayloadBlocking final
    : public SingleShotAsyncPayload<CallerOwnedPayloadBlocking<Func>> {
  static_assert(!std::is_reference_v<Func>);

 public:
  explicit CallerOwnedPayloadBlocking(Func& func) noexcept : func_(func) {}

  void DoPerformAndRelease() {
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

class Thread;

class ThreadControlBase {
 public:
  struct ev_loop* GetEvLoop() const noexcept;

  /// Fast non allocating function to execute a `func(*data)` in EvLoop.
  void RunPayloadInEvLoopAsync(AsyncPayloadBase& payload) noexcept;

  /// Fast non allocating function to register a `func(*data)` in EvLoop.
  /// Does not wake up the ev thread immediately as an optimization.
  /// Depending on thread settings might fallback to RunPayloadInEvLoopAsync.
  void RunPayloadInEvLoopDeferred(AsyncPayloadBase& payload,
                                  Deadline deadline) noexcept;

  /// Allocating function to execute `func()` in EvLoop.
  template <typename Func>
  void RunInEvLoopAsync(Func&& func);

  /// Allocating function to execute `func()` in EvLoop.
  /// Does not wake up the ev thread immediately as an optimization.
  template <typename Func>
  void RunInEvLoopDeferred(Func&& func);

  template <typename Func>
  void RunInEvLoopSync(Func&& func);

  template <typename Func>
  void RunInEvLoopBlocking(Func&& func);

  bool IsInEvThread() const noexcept;

  std::uint8_t GetCurrentLoadPercent() const;
  const std::string& GetName() const;

 protected:
  explicit ThreadControlBase(Thread& thread) noexcept;

  void DoStart(ev_timer& w) noexcept;
  void DoStop(ev_timer& w) noexcept;
  void DoAgain(ev_timer& w) noexcept;

  void DoStart(ev_async& w) noexcept;
  void DoStop(ev_async& w) noexcept;
  void DoSend(ev_async& w) noexcept;

  void DoStart(ev_io& w) noexcept;
  void DoStop(ev_io& w) noexcept;

 private:
  Thread& thread_;
};

template <typename Func>
void ThreadControlBase::RunInEvLoopAsync(Func&& func) {
  if (IsInEvThread()) {
    func();
    return;
  }

  using Payload = impl::UniquePayloadAsync<std::remove_reference_t<Func>>;
  auto payload = std::make_unique<Payload>(std::forward<Func>(func));

  RunPayloadInEvLoopAsync(*payload.release());
}

template <typename Func>
void ThreadControlBase::RunInEvLoopDeferred(Func&& func) {
  if (IsInEvThread()) {
    func();
    return;
  }

  using Payload = impl::UniquePayloadAsync<std::remove_reference_t<Func>>;
  auto payload = std::make_unique<Payload>(std::forward<Func>(func));

  RunPayloadInEvLoopDeferred(*payload.release(), {});
}

template <typename Func>
void ThreadControlBase::RunInEvLoopSync(Func&& func) {
  if (IsInEvThread()) {
    func();
    return;
  }

  using Payload = impl::CallerOwnedPayloadSync<std::remove_reference_t<Func>>;
  Payload payload{func};

  RunPayloadInEvLoopAsync(payload);

  payload.Wait();
}

template <typename Func>
void ThreadControlBase::RunInEvLoopBlocking(Func&& func) {
  if (IsInEvThread()) {
    func();
    return;
  }

  using Payload =
      impl::CallerOwnedPayloadBlocking<std::remove_reference_t<Func>>;
  Payload payload{func};

  RunPayloadInEvLoopAsync(payload);

  payload.Wait();
}

class TimerThreadControl final : public ThreadControlBase {
 public:
  explicit TimerThreadControl(Thread& thread) noexcept;

  void Start(ev_timer& w) noexcept;
  void Stop(ev_timer& w) noexcept;
  void Again(ev_timer& w) noexcept;
};

class ThreadControl final : public ThreadControlBase {
 public:
  explicit ThreadControl(Thread& thread) noexcept;

  void Start(ev_timer& w) noexcept;
  void Stop(ev_timer& w) noexcept;
  void Again(ev_timer& w) noexcept;

  void Start(ev_async& w) noexcept;
  void Stop(ev_async& w) noexcept;
  void Send(ev_async& w) noexcept;

  void Start(ev_io& w) noexcept;
  void Stop(ev_io& w) noexcept;
};

}  // namespace engine::ev

USERVER_NAMESPACE_END
