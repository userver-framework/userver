#include <engine/ev/watcher.hpp>

#include <fcntl.h>
#include <unistd.h>

#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::ev {
namespace {
[[maybe_unused]] bool IsFdValid(int fd) { return ::fcntl(fd, F_GETFD) != -1; }
}  // namespace

template <>
void Watcher<ev_async>::Init(void (*cb)(struct ev_loop*, ev_async*,
                                        int) noexcept) {
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast)
  ev_async_init(&w_, cb);
}

template <>
void Watcher<ev_async>::StartImpl() {
  if (is_running_) return;
  is_running_ = true;
  thread_control_.Start(w_);
}

template <>
void Watcher<ev_async>::StopImpl() {
  if (!is_running_) return;
  is_running_ = false;
  thread_control_.Stop(w_);
}

template <>
void Watcher<ev_io>::Init(void (*cb)(struct ev_loop*, ev_io*, int) noexcept,
                          int fd, int events) {
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast)
  ev_io_init(&w_, cb, fd, events);
}

template <>
void Watcher<ev_io>::Init(void (*cb)(struct ev_loop*, ev_io*, int) noexcept) {
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast)
  ev_init(&w_, cb);
}

template <>
template <>
void Watcher<ev_io>::Set(int fd, int events) {
  ev_io_set(&w_, fd, events);
}

template <>
void Watcher<ev_io>::StartImpl() {
  if (is_running_) return;
  is_running_ = true;
  UASSERT_MSG(IsFdValid(w_.fd), "Invalid fd=" + std::to_string(w_.fd));
  thread_control_.Start(w_);
}

template <>
void Watcher<ev_io>::StopImpl() {
  if (!is_running_) return;
  is_running_ = false;
  UASSERT_MSG(IsFdValid(w_.fd), "Invalid fd=" + std::to_string(w_.fd));
  thread_control_.Stop(w_);
}

template <>
void Watcher<ev_timer>::Init(void (*cb)(struct ev_loop*, ev_timer*,
                                        int) noexcept,
                             LibEvDuration after, LibEvDuration repeat) {
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast)
  ev_timer_init(&w_, cb, after.count(), repeat.count());
}

template <>
template <>
void Watcher<ev_timer>::Set(LibEvDuration after, LibEvDuration repeat) {
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast)
  ev_timer_set(&w_, after.count(), repeat.count());
}

template <>
void Watcher<ev_timer>::StartImpl() {
  if (is_running_) return;
  is_running_ = true;
  thread_control_.Start(w_);
}

template <>
void Watcher<ev_timer>::StopImpl() {
  if (!is_running_) return;
  is_running_ = false;
  thread_control_.Stop(w_);
}

template <>
template <>
void Watcher<ev_timer>::AgainImpl() {
  is_running_ = true;
  thread_control_.Again(w_);
}

}  // namespace engine::ev

USERVER_NAMESPACE_END
