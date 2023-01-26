#pragma once

#include <atomic>
#include <type_traits>

#include <ev.h>

#include <engine/ev/async_payload_base.hpp>
#include <engine/ev/thread_control.hpp>

#include <userver/utils/assert.hpp>
#include <userver/utils/fast_scope_guard.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::ev {

using LibEvDuration = std::chrono::duration<double>;

/// An ev watcher wrapper. Usable from coroutines and from the bound ev thread.
template <typename EvType>
class Watcher final : public MultiShotAsyncPayload<Watcher<EvType>> {
 public:
  template <typename Obj>
  Watcher(const ThreadControl& thread_control, Obj* data);

  ~Watcher();

  void Init(void (*cb)(struct ev_loop*, ev_async*, int) noexcept);
  void Init(void (*cb)(struct ev_loop*, ev_io*, int) noexcept);
  void Init(void (*cb)(struct ev_loop*, ev_io*, int) noexcept, int fd,
            int events);  // TODO: use utils::Flags for events
  void Init(void (*cb)(struct ev_loop*, ev_timer*, int) noexcept,
            LibEvDuration after, LibEvDuration repeat);

  template <typename T = EvType>
  std::enable_if_t<std::is_same_v<T, ev_io>> Set(int fd, int events);

  template <typename T = EvType>
  std::enable_if_t<std::is_same_v<T, ev_timer>> Set(LibEvDuration after,
                                                    LibEvDuration repeat);

  // Synchronously start/stop ev_xxx. Can be used from coroutines only.
  void Start();
  void Stop();

  // Asynchronously start ev_xxx.
  void StartAsync();

  // Asynchronously stop ev_xxx. Beware of dangling references!
  void StopAsync();

  template <typename T = EvType>
  std::enable_if_t<std::is_same_v<T, ev_timer>> Again();

  template <typename T = EvType>
  std::enable_if_t<std::is_same_v<T, ev_async>> Send();

  // Schedule an arbitrary function on the bound ev thread.
  // The Watcher won't be destroyed until the operation is complete.
  // The operations launched this way may be reordered with respect to
  // StartAsync and StopAsync.
  template <typename Function>
  void RunInBoundEvLoopAsync(Function&&);

  template <typename Function>
  void RunInBoundEvLoopSync(Function&&);

 private:
  friend class MultiShotAsyncPayload<Watcher<EvType>>;

  enum class AsyncOpType : std::uint32_t { kNone, kStart, kStop };

  auto EvLoopOpsCountingGuard() noexcept;
  void DoPerformAndRelease();
  void PushAsyncOp(AsyncOpType op);
  bool IsActive() const noexcept;

  void StartImpl();
  void StopImpl();

  template <typename T = EvType>
  std::enable_if_t<std::is_same_v<T, ev_timer>> AgainImpl();

  EvType w_;
  ThreadControl thread_control_;
  std::atomic<bool> is_running_{false};
  std::atomic<AsyncOpType> pending_async_op_{{}};
  std::atomic<std::size_t> pending_op_count_{0};
};

template <typename EvType>
template <typename Obj>
Watcher<EvType>::Watcher(const ThreadControl& thread_control, Obj* data)
    : thread_control_(thread_control) {
  w_.data = static_cast<void*>(data);
  UASSERT(!IsActive());
}

template <typename EvType>
Watcher<EvType>::~Watcher() {
  Stop();
  UASSERT(!IsActive());
}

template <typename EvType>
void Watcher<EvType>::Start() {
  RunInBoundEvLoopSync([this] { StartImpl(); });
}

template <typename EvType>
void Watcher<EvType>::Stop() {
  if (!IsActive()) return;
  RunInBoundEvLoopSync([this] { StopImpl(); });
}

template <typename EvType>
void Watcher<EvType>::StartAsync() {
  PushAsyncOp(AsyncOpType::kStart);
}

template <typename EvType>
void Watcher<EvType>::StopAsync() {
  if (!IsActive()) return;
  PushAsyncOp(AsyncOpType::kStop);
}

template <typename EvType>
template <typename T>
std::enable_if_t<std::is_same_v<T, ev_timer>> Watcher<EvType>::Again() {
  RunInBoundEvLoopSync([this] { AgainImpl(); });
}

template <typename EvType>
template <typename T>
std::enable_if_t<std::is_same_v<T, ev_async>> Watcher<EvType>::Send() {
  thread_control_.Send(w_);
}

template <typename EvType>
template <typename Function>
void Watcher<EvType>::RunInBoundEvLoopAsync(Function&& func) {
  thread_control_.RunInEvLoopAsync(
      [guard = EvLoopOpsCountingGuard(), func = std::forward<Function>(func)] {
        func();
      });
}

template <typename EvType>
template <typename Function>
void Watcher<EvType>::RunInBoundEvLoopSync(Function&& func) {
  // We need guard here to make sure that ~Watcher() does not
  // return as long as we are calling Watcher::Stop or Watcher::Start from ev
  // thread.
  [[maybe_unused]] auto guard = EvLoopOpsCountingGuard();
  thread_control_.RunInEvLoopSync(std::forward<Function>(func));
}

template <typename EvType>
auto Watcher<EvType>::EvLoopOpsCountingGuard() noexcept {
  ++pending_op_count_;
  return utils::FastScopeGuard([this]() noexcept { --pending_op_count_; });
}

template <typename EvType>
void Watcher<EvType>::DoPerformAndRelease() {
  utils::FastScopeGuard release_guard([this]() noexcept {
    // As long as pending_op_count_ != 0, Watcher owner will not destroy it
    // immediately, instead it will launch a synchronous Stop operation that
    // will be scheduled after the current one.
    --pending_op_count_;
    // Watcher may be dead at this point.
  });

  const auto op = pending_async_op_.exchange({}, std::memory_order_relaxed);

  switch (op) {
    case AsyncOpType::kNone:
      break;
    case AsyncOpType::kStart:
      StartImpl();
      break;
    case AsyncOpType::kStop:
      StopImpl();
      break;
    default:
      UINVARIANT(false, "Invalid LifetimeOp value");
  }
}

template <typename EvType>
void Watcher<EvType>::PushAsyncOp(AsyncOpType op) {
  pending_async_op_.store(op, std::memory_order_relaxed);
  if (this->PrepareEnqueue()) {
    ++pending_op_count_;
    thread_control_.RunPayloadInEvLoopDeferred(*this, {});
  }
}

template <typename EvType>
bool Watcher<EvType>::IsActive() const noexcept {
  return pending_op_count_ != 0 || is_running_;
}

}  // namespace engine::ev

USERVER_NAMESPACE_END
