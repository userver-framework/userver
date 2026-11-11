#include <ugrpc/impl/event_engine/ev_epoll1_linux.hpp>

#if defined(ARCADIA_ROOT) && defined(__linux__)

#include <fmt/format.h>
#include <y_absl/status/status.h>
#include <y_absl/strings/string_view.h>

#include <cerrno>
#include <cstdint>
#include <cstring>
#include <utility>

#include <sys/socket.h>
#include <unistd.h>

#include <userver/engine/io/exception.hpp>
#include <userver/logging/log.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::impl::event_engine {

namespace {

constexpr std::uint32_t kReadInterest = EPOLLIN | EPOLLET;
constexpr std::uint32_t kReadWriteInterest = EPOLLIN | EPOLLOUT | EPOLLET;

int EpollCreateAndCloexec() {
    const int fd = ::epoll_create1(EPOLL_CLOEXEC);
    if (fd < 0) {
        throw engine::io::IoSystemError(errno, "creating the EventEngine epoll set");
    }
    return fd;
}

}  // namespace

Epoll1EventHandle::Epoll1EventHandle(Epoll1Poller& poller) noexcept : poller_(&poller) {}

void Epoll1EventHandle::ReInit(int fd) noexcept {
    fd_ = fd;
    read_closure_.InitEvent();
    write_closure_.InitEvent();
    write_interest_ = WriteInterest::kDisabled;
}

y_absl::Status Epoll1EventHandle::SetWriteInterest(WriteInterest interest) noexcept {
    if (write_interest_ == interest || IsHandleShutdown()) {
        return y_absl::OkStatus();
    }
    const bool enabled = interest == WriteInterest::kEnabled;
    epoll_event ev{};
    ev.events = enabled ? kReadWriteInterest : kReadInterest;
    ev.data.ptr = this;
    if (::epoll_ctl(poller_->g_epoll_set_.epfd, EPOLL_CTL_MOD, fd_, &ev) != 0) {
        return y_absl::UnavailableError(
            fmt::format("{} EPOLLOUT failed: {}", enabled ? "Arming" : "Releasing", std::strerror(errno))
        );
    }
    write_interest_ = interest;
    return y_absl::OkStatus();
}

void Epoll1EventHandle::NotifyOnWrite(PosixEngineClosure* on_write) noexcept {
    auto status = SetWriteInterest(WriteInterest::kEnabled);
    if (!status.ok()) {
        on_write->SetStatus(std::move(status));
        on_write->Run();
        return;
    }
    write_closure_.NotifyOn(on_write);
}

void Epoll1EventHandle::ReleaseWriteInterest() noexcept {
    if (const auto status = SetWriteInterest(WriteInterest::kDisabled); !status.ok()) {
        LOG_LIMITED_WARNING() << status.message() << "; EPOLLOUT stays armed for fd " << fd_;
    }
}

void Epoll1EventHandle::OrphanHandle(int* release_fd, y_absl::string_view reason) noexcept {
    const bool is_release_fd = (release_fd != nullptr);
    bool shut_down_here = false;
    if (!read_closure_.IsShutdown()) {
        shut_down_here = true;
        HandleShutdownInternal(
            y_absl::Status(y_absl::StatusCode::kUnknown, reason),
            is_release_fd ? FdDisposition::kRelease : FdDisposition::kClose
        );
    }
    if (is_release_fd) {
        if (!shut_down_here) {
            epoll_event phony_event{};
            if (::epoll_ctl(poller_->g_epoll_set_.epfd, EPOLL_CTL_DEL, fd_, &phony_event) != 0) {
                LOG_LIMITED_ERROR() << "OrphanHandle: epoll_ctl failed: " << std::strerror(errno);
            }
        }
        *release_fd = fd_;
    } else {
        ::shutdown(fd_, SHUT_RDWR);
        ::close(fd_);
    }
    ReturnToFreeList();
}

void Epoll1EventHandle::ReturnToFreeList() noexcept {
    read_closure_.DestroyEvent();
    write_closure_.DestroyEvent();
    UASSERT(poller_->handles_);
    poller_->handles_->Release(*this);
}

void Epoll1EventHandle::HandleShutdownInternal(y_absl::Status why, FdDisposition disposition) noexcept {
    if (read_closure_.SetShutdown(why)) {
        if (disposition == FdDisposition::kRelease) {
            epoll_event phony_event{};
            if (::epoll_ctl(poller_->g_epoll_set_.epfd, EPOLL_CTL_DEL, fd_, &phony_event) != 0) {
                LOG_LIMITED_ERROR() << "HandleShutdownInternal: epoll_ctl failed: " << std::strerror(errno);
            }
        }
        write_closure_.SetShutdown(std::move(why));
    }
}

void Epoll1EventHandle::ShutdownHandle(y_absl::Status why) noexcept {
    HandleShutdownInternal(std::move(why), FdDisposition::kClose);
}

Epoll1Poller::Epoll1Poller()
    : g_epoll_set_{EpollCreateAndCloexec(), {}}
{}

Epoll1Poller::~Epoll1Poller() { Shutdown(); }

void Epoll1Poller::Shutdown() noexcept {
    if (!handles_) {
        return;
    }
    handles_->Walk([](const Epoll1EventHandle& handle) {
        UINVARIANT(
            handle.read_closure_.IsDestroyed(),
            "Destroy every gRPC endpoint and listener before stopping the PosixEventEngine"
        );
    });
    ::close(std::exchange(g_epoll_set_.epfd, engine::io::kInvalidFd));
    handles_.reset();
}

Epoll1EventHandle* Epoll1Poller::CreateHandle(int fd) {
    if (!handles_) {
        throw engine::io::IoSystemError(EBADF, "Cannot register a descriptor after EventEngine poller shutdown");
    }
    auto* const new_handle = &handles_->Acquire([this] { return Epoll1EventHandle{*this}; });
    new_handle->ReInit(fd);
    epoll_event ev{};
    ev.events = kReadInterest;
    ev.data.ptr = new_handle;
    if (::epoll_ctl(g_epoll_set_.epfd, EPOLL_CTL_ADD, fd, &ev) != 0) {
        const int error = errno;
        new_handle->ShutdownHandle(y_absl::UnavailableError("epoll_ctl(EPOLL_CTL_ADD) failed"));
        new_handle->ReturnToFreeList();
        throw engine::io::IoSystemError(error, "registering a descriptor in the EventEngine epoll set");
    }
    return new_handle;
}

std::size_t Epoll1Poller::Work() noexcept {
    int r = 0;
    do {
        r = ::epoll_wait(g_epoll_set_.epfd, g_epoll_set_.events, kMaxEpollEvents, 0);
    } while (r < 0 && errno == EINTR);
    UINVARIANT(r >= 0, fmt::format("Epoll1Poller encountered an epoll_wait error: {}", std::strerror(errno)));
    for (int idx = 0; idx < r; ++idx) {
        const struct epoll_event& ev = g_epoll_set_.events[idx];
        auto* const handle = static_cast<Epoll1EventHandle*>(ev.data.ptr);
        const bool cancel = (ev.events & EPOLLHUP) != 0;
        const bool error = (ev.events & EPOLLERR) != 0;
        const bool read_ev = (ev.events & (EPOLLIN | EPOLLPRI)) != 0;
        const bool write_ev = (ev.events & EPOLLOUT) != 0;
        if (read_ev || cancel || error) {
            handle->SetReadable();
        }
        if (write_ev || cancel || error) {
            handle->SetWritable();
        }
    }
    return static_cast<std::size_t>(r);
}

}  // namespace ugrpc::impl::event_engine

USERVER_NAMESPACE_END

#endif
