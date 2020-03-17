#pragma once

#include <atomic>

#include <ev.h>

#include <engine/future.hpp>
#include "thread_control.hpp"

namespace engine::ev {

template <typename EvType>
class Watcher final : public ThreadControl {
 public:
  template <typename Obj>
  Watcher(const ThreadControl& thread_control, Obj* data);
  // NOLINTNEXTLINE(bugprone-exception-escape)
  ~Watcher() override;

  void Init(void (*cb)(struct ev_loop*, ev_async*, int) noexcept);
  void Init(void (*cb)(struct ev_loop*, ev_io*, int) noexcept);
  void Init(void (*cb)(struct ev_loop*, ev_io*, int) noexcept, int fd,
            int events);  // TODO: use utils::Flags for events
  void Init(void (*cb)(struct ev_loop*, ev_timer*, int) noexcept,
            ev_tstamp after, ev_tstamp repeat);
  void Init(void (*cb)(struct ev_loop*, ev_idle*, int) noexcept);

  template <typename T = EvType>
  typename std::enable_if<std::is_same<T, ev_io>::value, void>::type Set(
      int fd, int events);

  template <typename T = EvType>
  typename std::enable_if<std::is_same<T, ev_timer>::value, void>::type Set(
      ev_tstamp after, ev_tstamp repeat);

  /* Synchronously start/stop ev_xxx.  Can be used from coroutines only */
  void Start();
  void Stop();

  /* Asynchronously start ev_xxx. Must not block. */
  void StartAsync();

  /* Asynchronously stop ev_xxx. Beware of dangling references! */
  void StopAsync();

  template <typename T = EvType>
  typename std::enable_if<std::is_same<T, ev_timer>::value, void>::type Again();

  template <typename T = EvType>
  typename std::enable_if<std::is_same<T, ev_async>::value, void>::type Send();

 private:
  void StartImpl();
  void StopImpl();

  template <typename T = EvType>
  typename std::enable_if<std::is_same<T, ev_timer>::value, void>::type
  AgainImpl();

  template <typename T = EvType>
  typename std::enable_if<std::is_same<T, ev_async>::value, void>::type
  SendImpl();

  template <void (Watcher::*func)()>
  void CallInEvLoop();

  EvType w_;
  std::atomic<int> pending_async_ops_;
  std::atomic<bool> is_running_;
};

template <typename EvType>
template <typename Obj>
Watcher<EvType>::Watcher(const ThreadControl& thread_control, Obj* data)
    : ThreadControl(thread_control), pending_async_ops_(0), is_running_(false) {
  w_.data = static_cast<void*>(data);
}

template <typename EvType>
Watcher<EvType>::~Watcher() {
  Stop();
}

template <typename EvType>
void Watcher<EvType>::Start() {
  CallInEvLoop<&Watcher::StartImpl>();
}

template <typename EvType>
void Watcher<EvType>::Stop() {
  if (!pending_async_ops_ && !is_running_) return;

  CallInEvLoop<&Watcher::StopImpl>();
}

template <typename EvType>
void Watcher<EvType>::StartAsync() {
  ++pending_async_ops_;
  RunInEvLoopAsync([this] {
    StartImpl();
    --pending_async_ops_;
  });
}

template <typename EvType>
void Watcher<EvType>::StopAsync() {
  // no fetch_add in predicate is fine
  if (!pending_async_ops_ && !is_running_) return;

  ++pending_async_ops_;
  RunInEvLoopAsync([this] {
    StopImpl();
    --pending_async_ops_;
  });
}

template <typename EvType>
template <typename T>
typename std::enable_if<std::is_same<T, ev_timer>::value, void>::type
Watcher<EvType>::Again() {
  CallInEvLoop<&Watcher::AgainImpl>();
}

template <typename EvType>
template <typename T>
typename std::enable_if<std::is_same<T, ev_async>::value, void>::type
Watcher<EvType>::Send() {
  ev_async_send(GetEvLoop(), &w_);
}

template <typename EvType>
template <void (Watcher<EvType>::*func)()>
void Watcher<EvType>::CallInEvLoop() {
  // We need pending_async_ops_ here to make sure that ~Watcher() does not
  // return as long as we are calling Watcher::Stop or Watcher::Start from ev
  // thread.
  ++pending_async_ops_;
  RunInEvLoopSync([this] { (this->*func)(); });
  --pending_async_ops_;
}

}  // namespace engine::ev
