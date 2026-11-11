#pragma once

#if defined(ARCADIA_ROOT) && defined(__linux__)

#include <y_absl/status/status.h>
#include <y_absl/strings/string_view.h>

#include <cstddef>
#include <optional>

#include <sys/epoll.h>

#include <concurrent/intrusive_walkable_pool.hpp>
#include <userver/concurrent/impl/intrusive_hooks.hpp>
#include <userver/engine/io/common.hpp>

#include <ugrpc/impl/event_engine/closure.hpp>
#include <ugrpc/impl/event_engine/lockfree_event.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::impl::event_engine {

class Epoll1Poller;

class Epoll1EventHandle final {
public:
    explicit Epoll1EventHandle(Epoll1Poller& poller) noexcept;
    Epoll1EventHandle(const Epoll1EventHandle&) = delete;
    Epoll1EventHandle& operator=(const Epoll1EventHandle&) = delete;

    void ReInit(int fd) noexcept;
    int WrappedFd() const noexcept { return fd_; }
    void OrphanHandle(int* release_fd, y_absl::string_view reason) noexcept;
    void ShutdownHandle(y_absl::Status why) noexcept;
    void NotifyOnRead(PosixEngineClosure* on_read) noexcept { read_closure_.NotifyOn(on_read); }
    void NotifyOnWrite(PosixEngineClosure* on_write) noexcept;
    void ReleaseWriteInterest() noexcept;
    void SetReadable() noexcept { read_closure_.SetReady(); }
    void SetWritable() noexcept { write_closure_.SetReady(); }
    bool IsHandleShutdown() const noexcept { return read_closure_.IsShutdown(); }

private:
    friend class Epoll1Poller;

    enum class WriteInterest { kDisabled, kEnabled };
    enum class FdDisposition { kClose, kRelease };

    void HandleShutdownInternal(y_absl::Status why, FdDisposition disposition) noexcept;
    void ReturnToFreeList() noexcept;
    y_absl::Status SetWriteInterest(WriteInterest interest) noexcept;

    int fd_{engine::io::kInvalidFd};
    Epoll1Poller* poller_;
    LockfreeEvent read_closure_;
    LockfreeEvent write_closure_;
    WriteInterest write_interest_{WriteInterest::kDisabled};
    concurrent::impl::IntrusiveWalkablePoolHook<Epoll1EventHandle> pool_hook_;
};

class Epoll1Poller final {
public:
    Epoll1Poller();
    ~Epoll1Poller();
    Epoll1Poller(const Epoll1Poller&) = delete;
    Epoll1Poller& operator=(const Epoll1Poller&) = delete;

    Epoll1EventHandle* CreateHandle(int fd);
    std::size_t Work() noexcept;
    int EpollFd() const noexcept { return g_epoll_set_.epfd; }
    void Shutdown() noexcept;

private:
    friend class Epoll1EventHandle;

    static constexpr int kMaxEpollEvents = 100;
    using HandlePool = concurrent::impl::IntrusiveWalkablePool<
        Epoll1EventHandle,
        concurrent::impl::MemberHook<&Epoll1EventHandle::pool_hook_>,
        offsetof(Epoll1EventHandle, pool_hook_)>;

    struct EpollSet {
        int epfd;
        struct epoll_event events[kMaxEpollEvents];
    };

    EpollSet g_epoll_set_;
    std::optional<HandlePool> handles_{std::in_place};
};

}  // namespace ugrpc::impl::event_engine

USERVER_NAMESPACE_END

#endif
