#include "watcher.hpp"

namespace engine {

template <>
void Watcher<ev_async>::Init(void (*cb)(struct ev_loop*, ev_async*, int)) {
  assert(!IsRunning());
  ev_async_init(&w_, cb);
}

template <>
void Watcher<ev_async>::StartImpl() {
  ev_async_start(GetEvLoop(), &w_);
}

template <>
void Watcher<ev_async>::StopImpl() {
  ev_async_stop(GetEvLoop(), &w_);
}

template <>
void Watcher<ev_io>::Init(void (*cb)(struct ev_loop*, ev_io*, int), int fd,
                          int events) {
  assert(!IsRunning());
  ev_io_init(&w_, cb, fd, events);
}

template <>
template <>
void Watcher<ev_io>::Set(int fd, int events) {
  ev_io_set(&w_, fd, events);
}

template <>
void Watcher<ev_io>::StartImpl() {
  ev_io_start(GetEvLoop(), &w_);
}

template <>
void Watcher<ev_io>::StopImpl() {
  ev_io_stop(GetEvLoop(), &w_);
}

template <>
void Watcher<ev_timer>::Init(void (*cb)(struct ev_loop*, ev_timer*, int),
                             ev_tstamp after, ev_tstamp repeat) {
  assert(!IsRunning());
  ev_timer_init(&w_, cb, after, repeat);
}

template <>
template <>
void Watcher<ev_timer>::Set(ev_tstamp after, ev_tstamp repeat) {
  ev_timer_set(&w_, after, repeat);
}

template <>
void Watcher<ev_timer>::StartImpl() {
  ev_timer_start(GetEvLoop(), &w_);
}

template <>
void Watcher<ev_timer>::StopImpl() {
  ev_timer_stop(GetEvLoop(), &w_);
}

template <>
template <>
void Watcher<ev_timer>::AgainImpl() {
  ev_timer_again(GetEvLoop(), &w_);
}

template <>
void Watcher<ev_idle>::Init(void (*cb)(struct ev_loop*, ev_idle*, int)) {
  assert(!IsRunning());
  ev_idle_init(&w_, cb);
}

template <>
void Watcher<ev_idle>::StartImpl() {
  ev_idle_start(GetEvLoop(), &w_);
}

template <>
void Watcher<ev_idle>::StopImpl() {
  ev_idle_stop(GetEvLoop(), &w_);
}

}  // namespace engine
