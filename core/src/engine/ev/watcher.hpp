#pragma once

#include <atomic>
#include <type_traits>

#include <ev.h>

#include <engine/ev/async_payload_base.hpp>
#include <engine/ev/thread_control.hpp>

#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::ev {

/// Watcher type for a particular event type. Not intended for ownage by
/// intrusive_ptr.
template <typename EvType>
class Watcher final : private AsyncPayloadBase {
 public:
  template <typename Obj>
  Watcher(const ThreadControl& thread_control, Obj* data);

  ~Watcher();

  void Init(void (*cb)(struct ev_loop*, ev_async*, int) noexcept);
  void Init(void (*cb)(struct ev_loop*, ev_io*, int) noexcept);
  void Init(void (*cb)(struct ev_loop*, ev_io*, int) noexcept, int fd,
            int events);  // TODO: use utils::Flags for events
  void Init(void (*cb)(struct ev_loop*, ev_timer*, int) noexcept,
            ev_tstamp after, ev_tstamp repeat);
  void Init(void (*cb)(struct ev_loop*, ev_idle*, int) noexcept);

  template <typename T = EvType>
  std::enable_if_t<std::is_same_v<T, ev_io>> Set(int fd, int events);

  template <typename T = EvType>
  std::enable_if_t<std::is_same_v<T, ev_timer>> Set(ev_tstamp after,
                                                    ev_tstamp repeat);

  /* Synchronously start/stop ev_xxx.  Can be used from coroutines only */
  void Start();
  void Stop();

  /* Asynchronously start ev_xxx. Must not block. */
  void StartAsync();

  /* Asynchronously stop ev_xxx. Beware of dangling references! */
  void StopAsync();

  template <typename T = EvType>
  std::enable_if_t<std::is_same_v<T, ev_timer>> Again();

  template <typename T = EvType>
  std::enable_if_t<std::is_same_v<T, ev_async>> Send();

  template <typename Function>
  void RunInBoundEvLoopAsync(Function&&);

  template <typename Function>
  void RunInBoundEvLoopSync(Function&&);

 private:
  static void Release(AsyncPayloadBase& base) noexcept;

  AsyncPayloadPtr SelfAsPayload() noexcept;

  void StartImpl();
  void StopImpl();

  template <typename T = EvType>
  std::enable_if_t<std::is_same_v<T, ev_timer>> AgainImpl();

  bool HasPendingEvLoopOps() const noexcept { return pending_op_count_ != 0; }

  EvType w_;
  ThreadControl thread_control_;
  std::atomic<bool> is_running_{false};
  std::atomic<std::size_t> pending_op_count_{0};
};

template <typename EvType>
template <typename Obj>
Watcher<EvType>::Watcher(const ThreadControl& thread_control, Obj* data)
    : AsyncPayloadBase(&Release), thread_control_(thread_control) {
  w_.data = static_cast<void*>(data);
  UASSERT(!HasPendingEvLoopOps());
}

template <typename EvType>
Watcher<EvType>::~Watcher() {
  Stop();
  UASSERT(!HasPendingEvLoopOps());
}

template <typename EvType>
void Watcher<EvType>::Start() {
  RunInBoundEvLoopSync([this] { StartImpl(); });
}

template <typename EvType>
void Watcher<EvType>::Stop() {
  if (!HasPendingEvLoopOps() && !is_running_) return;

  RunInBoundEvLoopSync([this] { StopImpl(); });
}

template <typename EvType>
void Watcher<EvType>::StartAsync() {
  thread_control_.RunInEvLoopDeferred(
      [](AsyncPayloadPtr&& ptr) {
        auto& self = static_cast<Watcher&>(*ptr);
        self.StartImpl();
      },
      SelfAsPayload(), {});
}

template <typename EvType>
void Watcher<EvType>::StopAsync() {
  // no fetch_add in predicate is fine
  if (!HasPendingEvLoopOps() && !is_running_) return;

  thread_control_.RunInEvLoopAsync(
      [](AsyncPayloadPtr&& ptr) {
        auto& self = static_cast<Watcher&>(*ptr);
        self.StopImpl();
      },
      SelfAsPayload());
}

template <typename EvType>
template <typename T>
std::enable_if_t<std::is_same_v<T, ev_timer>> Watcher<EvType>::Again() {
  RunInBoundEvLoopSync([this] { AgainImpl(); });
}

template <typename EvType>
template <typename T>
std::enable_if_t<std::is_same_v<T, ev_async>> Watcher<EvType>::Send() {
  ev_async_send(thread_control_.GetEvLoop(), &w_);
}

template <typename EvType>
template <typename Function>
void Watcher<EvType>::RunInBoundEvLoopAsync(Function&& func) {
  thread_control_.RunInEvLoopAsync(
      [ev_loop_ops_counting_guard = SelfAsPayload(),
       func = std::forward<Function>(func)] { func(); });
}

template <typename EvType>
template <typename Function>
void Watcher<EvType>::RunInBoundEvLoopSync(Function&& func) {
  // We need guard here to make sure that ~Watcher() does not
  // return as long as we are calling Watcher::Stop or Watcher::Start from ev
  // thread.
  auto ev_loop_ops_counting_guard = SelfAsPayload();
  thread_control_.RunInEvLoopSync(std::forward<Function>(func));
}

template <typename EvType>
void Watcher<EvType>::Release(AsyncPayloadBase& base) noexcept {
  --static_cast<Watcher&>(base).pending_op_count_;
}

template <typename EvType>
AsyncPayloadPtr Watcher<EvType>::SelfAsPayload() noexcept {
  ++pending_op_count_;
  return AsyncPayloadPtr(this);
}

}  // namespace engine::ev

USERVER_NAMESPACE_END
