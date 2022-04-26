#pragma once

#include "thread.hpp"

#include <future>

#include <userver/engine/single_consumer_event.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/utils/make_intrusive_ptr.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::ev {

namespace impl {

template <class Func>
class RefcountedFunction : public IntrusiveRefcountedBase {
 public:
  explicit RefcountedFunction(Func&& func) : function_(std::move(func)) {}
  explicit RefcountedFunction(const Func& func) : function_(func) {}

  void Invoke() { function_(); }

 private:
  static_assert(!std::is_reference_v<Func>);
  Func function_;
};

template <class Func>
class RefcountedFunctionWithEvent final : public RefcountedFunction<Func> {
 public:
  using RefcountedFunction<Func>::RefcountedFunction;

  void Invoke() {
    RefcountedFunction<Func>::Invoke();
    event_.Send();
  }

  engine::SingleConsumerEvent& Event() { return event_; }

 private:
  engine::SingleConsumerEvent event_;
};

template <class Func>
class RefcountedFunctionWithPromise final : public RefcountedFunction<Func> {
 public:
  using RefcountedFunction<Func>::RefcountedFunction;

  void Invoke() {
    RefcountedFunction<Func>::Invoke();
    promise_.set_value();
  }

  std::promise<void>& Promise() { return promise_; }

 private:
  std::promise<void> promise_;
};

}  // namespace impl

class ThreadControl final {
 public:
  explicit ThreadControl(Thread& thread) : thread_(thread) {}
  ~ThreadControl() = default;

  inline struct ev_loop* GetEvLoop() const { return thread_.GetEvLoop(); }

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
  void RunInEvLoopAsync(OnRefcountedPayload* func,
                        boost::intrusive_ptr<IntrusiveRefcountedBase>&& data);

  /// Fast non allocating function to register a `func(*data)` in EvLoop.
  /// Depending on thread settings might fallback to RunInEvLoopAsync
  void RegisterTimerEventInEvLoop(
      OnRefcountedPayload* func,
      boost::intrusive_ptr<IntrusiveRefcountedBase>&& data, Deadline deadline);

  /// Allocating function to execute `func()` in EvLoop.
  ///
  /// Dynamically allocates storage and forwards func into it, passes storage
  /// and helper lambda into the two argument RunInEvLoopAsync overload.
  template <class Function>
  void RunInEvLoopAsync(Function&& func);

  template <class Function>
  void RunInEvLoopSync(Function&& func);

  // For redis
  template <class Function>
  void RunInEvLoopBlocking(Function&& func);

  bool IsInEvThread() const { return thread_.IsInEvThread(); }

 private:
  Thread& thread_;
};

template <class Function>
void ThreadControl::RunInEvLoopAsync(Function&& func) {
  if (IsInEvThread()) {
    func();
    return;
  }

  using FunctionType = std::remove_reference_t<Function>;
  using Refcounted = impl::RefcountedFunction<FunctionType>;
  auto data =
      utils::make_intrusive_ptr<Refcounted>(std::forward<Function>(func));
  RunInEvLoopAsync(
      [](IntrusiveRefcountedBase& data) {
        PolymorphicDowncast<Refcounted&>(data).Invoke();
      },
      std::move(data));
}

template <class Function>
void ThreadControl::RunInEvLoopSync(Function&& func) {
  if (IsInEvThread()) {
    func();
    return;
  }

  using FunctionType = std::remove_reference_t<Function>;
  using Refcounted = impl::RefcountedFunctionWithEvent<FunctionType>;
  auto data =
      utils::make_intrusive_ptr<Refcounted>(std::forward<Function>(func));
  RunInEvLoopAsync(
      [](IntrusiveRefcountedBase& data) {
        PolymorphicDowncast<Refcounted&>(data).Invoke();
      },
      data);

  engine::TaskCancellationBlocker cancellation_blocker;
  const auto result = data->Event().WaitForEvent();
  UASSERT(result);
}

template <class Function>
void ThreadControl::RunInEvLoopBlocking(Function&& func) {
  if (IsInEvThread()) {
    func();
    return;
  }

  using FunctionType = std::remove_reference_t<Function>;
  using Refcounted = impl::RefcountedFunctionWithPromise<FunctionType>;
  auto data =
      utils::make_intrusive_ptr<Refcounted>(std::forward<Function>(func));
  auto func_future = data->Promise().get_future();
  RunInEvLoopAsync(
      [](IntrusiveRefcountedBase& data) {
        PolymorphicDowncast<Refcounted&>(data).Invoke();
      },
      std::move(data));
  func_future.get();
}

}  // namespace engine::ev

USERVER_NAMESPACE_END
