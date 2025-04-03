#include <engine/ev/watcher.hpp>

#include <fcntl.h>
#include <unistd.h>
#include <string>

#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::ev {
namespace {
[[maybe_unused]] bool IsFdValid(int fd) noexcept { return ::fcntl(fd, F_GETFD) != -1; }
}  // namespace

template <>
void Watcher<ev_async>::Init(void (*cb)(struct ev_loop*, ev_async*, int) noexcept) noexcept {
    UASSERT_MSG(!is_running_, "Values in active watcher should not be changed.");
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast)
    ev_async_init(&w_, cb);
}

template <>
void Watcher<ev_async>::StartImpl() noexcept {
    if (is_running_) return;
    is_running_ = true;
    thread_control_.Start(w_);
}

template <>
void Watcher<ev_async>::StopImpl() noexcept {
    if (!is_running_) return;
    is_running_ = false;
    thread_control_.Stop(w_);
}

template <>
void Watcher<ev_io>::Init(void (*cb)(struct ev_loop*, ev_io*, int) noexcept, int fd, int events) noexcept {
    UASSERT_MSG(!is_running_, "Values in active watcher should not be changed.");
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast)
    ev_io_init(&w_, cb, fd, events);
}

template <>
void Watcher<ev_io>::Init(void (*cb)(struct ev_loop*, ev_io*, int) noexcept) noexcept {
    Init(cb, -1, 0);
}

template <>
template <>
void Watcher<ev_io>::Set(int fd, int events) noexcept {
    UASSERT_MSG(!is_running_, "Values in active watcher should not be changed.");
    ev_io_set(&w_, fd, events);
}

template <>
template <>
int Watcher<ev_io>::GetFd() const noexcept {
    // fd is marked with [read-only] attribute in libev. It is safe to read it
    return w_.fd;
}

template <>
void Watcher<ev_io>::StartImpl() noexcept {
    if (is_running_) return;
    is_running_ = true;
    UASSERT_MSG(IsFdValid(GetFd()), "Invalid fd=" + std::to_string(GetFd()));
    thread_control_.Start(w_);
}

template <>
void Watcher<ev_io>::StopImpl() noexcept {
    if (!is_running_) return;
    is_running_ = false;
    UASSERT_MSG(IsFdValid(GetFd()), "Invalid fd=" + std::to_string(GetFd()));
    thread_control_.Stop(w_);
}

template <>
void Watcher<ev_timer>::Init(
    void (*cb)(struct ev_loop*, ev_timer*, int) noexcept,
    LibEvDuration after,
    LibEvDuration repeat
) noexcept {
    UASSERT_MSG(!is_running_, "Values in active watcher should not be changed.");
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast)
    ev_timer_init(&w_, cb, after.count(), repeat.count());
}

template <>
template <>
void Watcher<ev_timer>::Set(LibEvDuration after, LibEvDuration repeat) noexcept {
    UASSERT_MSG(!is_running_, "Values in active watcher should not be changed.");
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast)
    ev_timer_set(&w_, after.count(), repeat.count());
}

template <>
void Watcher<ev_timer>::StartImpl() noexcept {
    if (is_running_) return;
    is_running_ = true;
    thread_control_.Start(w_);
}

template <>
void Watcher<ev_timer>::StopImpl() noexcept {
    if (!is_running_) return;
    is_running_ = false;
    thread_control_.Stop(w_);
}

template <>
template <>
void Watcher<ev_timer>::AgainImpl() noexcept {
    is_running_ = true;
    thread_control_.Again(w_);
}

}  // namespace engine::ev

USERVER_NAMESPACE_END
