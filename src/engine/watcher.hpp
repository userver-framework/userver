#pragma once

#include <ev.h>

#include <engine/ev/thread_control.hpp>
#include <engine/future.hpp>

namespace engine {

template <typename EvType>
class Watcher : public ev::ThreadControl {
 public:
  template <typename Obj>
  Watcher(const ev::ThreadControl& thread_control, Obj* data);
  virtual ~Watcher();

  void Init(void (*cb)(struct ev_loop*, ev_async*, int));
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

  void Start();
  void StartFromEvLoop();
  void Stop();
  void StopFromEvLoop();

  template <typename T = EvType>
  typename std::enable_if<std::is_same<T, ev_timer>::value, void>::type Again();

  template <typename T = EvType>
  typename std::enable_if<std::is_same<T, ev_async>::value, void>::type Send();

  bool IsRunning() const;

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
  bool is_running_ = false;
};

template <typename EvType>
template <typename Obj>
Watcher<EvType>::Watcher(const ev::ThreadControl& thread_control, Obj* data)
    : ev::ThreadControl(thread_control) {
  w_.data = static_cast<void*>(data);
}

template <typename EvType>
Watcher<EvType>::~Watcher() {
  if (is_running_) Stop();
}

template <typename EvType>
void Watcher<EvType>::Start() {
  is_running_ = true;
  CallInEvLoop<&Watcher::StartImpl>();
}

template <typename EvType>
void Watcher<EvType>::StartFromEvLoop() {
  if (is_running_) return;
  is_running_ = true;
  StartImpl();
}

template <typename EvType>
void Watcher<EvType>::Stop() {
  is_running_ = false;
  CallInEvLoop<&Watcher::StopImpl>();
}

template <typename EvType>
void Watcher<EvType>::StopFromEvLoop() {
  if (!is_running_) return;
  is_running_ = false;
  StopImpl();
}

template <typename EvType>
template <typename T>
typename std::enable_if<std::is_same<T, ev_timer>::value, void>::type
Watcher<EvType>::Again() {
  is_running_ = true;
  CallInEvLoop<&Watcher::AgainImpl>();
}

template <typename EvType>
template <typename T>
typename std::enable_if<std::is_same<T, ev_async>::value, void>::type
Watcher<EvType>::Send() {
  ev_async_send(GetEvLoop(), &w_);
}

template <typename EvType>
bool Watcher<EvType>::IsRunning() const {
  return is_running_;
}

template <typename EvType>
template <void (Watcher<EvType>::*func)()>
void Watcher<EvType>::CallInEvLoop() {
  auto promise = std::make_shared<Promise<void>>();
  auto future = promise->GetFuture();
  RunInEvLoopAsync([this, promise]() {
    (this->*func)();
    promise->SetValue();
  });
  future.Get();
}

}  // namespace engine
