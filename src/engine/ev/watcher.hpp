#pragma once

#include <ev.h>

#include <engine/future.hpp>
#include "thread_control.hpp"

namespace engine {
namespace ev {

template <typename EvType>
class Watcher : public ThreadControl {
 public:
  template <typename Obj>
  Watcher(const ThreadControl& thread_control, Obj* data);
  virtual ~Watcher();

  void Init(void (*cb)(struct ev_loop*, ev_async*, int));
  void Init(void (*cb)(struct ev_loop*, ev_io*, int));
  void Init(void (*cb)(struct ev_loop*, ev_io*, int), int fd,
            int events);  // TODO: use utils::Flags for events
  void Init(void (*cb)(struct ev_loop*, ev_timer*, int), ev_tstamp after,
            ev_tstamp repeat);
  void Init(void (*cb)(struct ev_loop*, ev_idle*, int));

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
  // no StopAsync(), you really might want to wait

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
  bool is_running_;
};

template <typename EvType>
template <typename Obj>
Watcher<EvType>::Watcher(const ThreadControl& thread_control, Obj* data)
    : ThreadControl(thread_control), is_running_(false) {
  w_.data = static_cast<void*>(data);
}

template <typename EvType>
Watcher<EvType>::~Watcher() {
  if (is_running_) Stop();
}

template <typename EvType>
void Watcher<EvType>::Start() {
  CallInEvLoop<&Watcher::StartImpl>();
}

template <typename EvType>
void Watcher<EvType>::Stop() {
  CallInEvLoop<&Watcher::StopImpl>();
}

template <typename EvType>
void Watcher<EvType>::StartAsync() {
  RunInEvLoopAsync([this] { StartImpl(); });
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
  if (IsInEvThread()) {
    (this->*func)();
    return;
  }

  // TODO: use WrappedCall
  auto promise = std::make_shared<Promise<void>>();
  auto future = promise->get_future();
  RunInEvLoopAsync([ this, promise = std::move(promise) ] {
    (this->*func)();
    promise->set_value();
  });
  future.get();
}

}  // namespace ev
}  // namespace engine
