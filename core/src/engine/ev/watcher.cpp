#include <engine/ev/watcher.hpp>

#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::ev {

template <>
void Watcher<ev_async>::Init(RawCallback<ev_async> cb) noexcept {
    UASSERT_MSG(!is_running_, "Values in active watcher should not be changed.");
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast)
    ev_async_init(&w_, cb);
}

template <>
void Watcher<ev_io>::Init(RawCallback<ev_io> cb, int fd, int events) noexcept {
    UASSERT_MSG(!is_running_, "Values in active watcher should not be changed.");
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast)
    ev_io_init(&w_, cb, fd, events);
}

template <>
void Watcher<ev_io>::Init(RawCallback<ev_io> cb) noexcept {
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
void Watcher<ev_timer>::Init(RawCallback<ev_timer> cb, LibEvDuration after, LibEvDuration repeat) noexcept {
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
template <>
void Watcher<ev_timer>::AgainImpl() noexcept {
    is_running_ = true;
    thread_control_.Again(w_);
}

}  // namespace engine::ev

USERVER_NAMESPACE_END
