#include <ugrpc/impl/event_engine/nonblocking_socket.hpp>

#if defined(ARCADIA_ROOT) && defined(__linux__)

#include <y_absl/status/status.h>
#include <y_absl/status/statusor.h>
#include <y_absl/strings/str_cat.h>
#include <y_absl/strings/string_view.h>

#include <algorithm>
#include <cerrno>
#include <climits>
#include <cstring>
#include <optional>
#include <utility>

#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <unistd.h>

#include <userver/engine/io/exception.hpp>
#include <userver/utils/assert.hpp>

#include <ugrpc/impl/event_engine/tcp_options.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::impl::event_engine {

namespace {

y_absl::Status ErrnoStatus(y_absl::string_view operation) noexcept {
    return y_absl::FailedPreconditionError(y_absl::StrCat(operation, ": ", std::strerror(errno)));
}

y_absl::StatusOr<NonblockingSocket::ResolvedAddress> AddressOf(
    int fd,
    int (*getname)(int, sockaddr*, socklen_t*),
    y_absl::string_view operation
) noexcept {
    sockaddr_storage storage{};
    socklen_t length = sizeof(storage);
    if (getname(fd, reinterpret_cast<sockaddr*>(&storage), &length) != 0) {
        return y_absl::InternalError(y_absl::StrCat(operation, ": ", std::strerror(errno)));
    }
    return NonblockingSocket::ResolvedAddress{reinterpret_cast<const sockaddr*>(&storage), length};
}

int CheckedFd(int fd) {
    if (fd < 0) {
        throw engine::io::IoException("Attempt to use a closed nonblocking socket");
    }
    return fd;
}

std::span<const iovec> TrimBuffers(std::span<const iovec> buffers) {
    while (!buffers.empty() && buffers.front().iov_len == 0) {
        buffers = buffers.subspan(1);
    }
    return buffers.first(std::min(buffers.size(), static_cast<std::size_t>(IOV_MAX)));
}

template <typename Syscall>
std::optional<std::size_t> NoblockIo(Syscall&& syscall, const char* operation) {
    for (;;) {
        const auto result = syscall();
        if (result >= 0) {
            return static_cast<std::size_t>(result);
        }
        const int error = errno;
        if (error == EINTR) {
            continue;
        }
        if (error == EAGAIN || error == EWOULDBLOCK) {
            return std::nullopt;
        }
        throw engine::io::IoSystemError(error, operation);
    }
}

}  // namespace

NonblockingSocket::NonblockingSocket(int fd) noexcept : fd_(fd) {
    UASSERT(fd >= 0);
#ifndef NDEBUG
    const int status_flags = ::fcntl(fd, F_GETFL);
    const int descriptor_flags = ::fcntl(fd, F_GETFD);
    UASSERT_MSG(
        status_flags != -1 && (status_flags & O_NONBLOCK) != 0,
        "NonblockingSocket adopts only O_NONBLOCK descriptors"
    );
    UASSERT_MSG(
        descriptor_flags != -1 && (descriptor_flags & FD_CLOEXEC) != 0,
        "NonblockingSocket adopts only FD_CLOEXEC descriptors"
    );
#endif
}

NonblockingSocket& NonblockingSocket::operator=(NonblockingSocket&& other) noexcept {
    if (this != &other) {
        Close();
        fd_ = std::exchange(other.fd_, kInvalidFd);
    }
    return *this;
}

std::optional<NonblockingSocket> NonblockingSocket::Create(int family) noexcept {
    const int fd = ::socket(family, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
    if (fd < 0) {
        return std::nullopt;
    }
    return NonblockingSocket{fd};
}

void NonblockingSocket::Close() noexcept {
    if (IsValid()) {
        ::close(std::exchange(fd_, kInvalidFd));
    }
}

y_absl::Status NonblockingSocket::SetSocketLowLatency(int low_latency) const noexcept {
    if (::setsockopt(fd_, IPPROTO_TCP, TCP_NODELAY, &low_latency, sizeof(low_latency)) != 0) {
        return ErrnoStatus("setsockopt(TCP_NODELAY)");
    }
    return y_absl::OkStatus();
}

y_absl::Status NonblockingSocket::SetSocketReuseAddr(int reuse) const noexcept {
    if (::setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) != 0) {
        return ErrnoStatus("setsockopt(SO_REUSEADDR)");
    }
    return y_absl::OkStatus();
}

y_absl::Status NonblockingSocket::SetSocketReusePort(int reuse) const noexcept {
    if (::setsockopt(fd_, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse)) != 0) {
        return ErrnoStatus("setsockopt(SO_REUSEPORT)");
    }
    return y_absl::OkStatus();
}

bool NonblockingSocket::SetSocketDualStack() const noexcept {
    const int off = 0;
    return ::setsockopt(fd_, IPPROTO_IPV6, IPV6_V6ONLY, &off, sizeof(off)) == 0;
}

y_absl::Status NonblockingSocket::SetSocketRcvBuf(int buffer_size_bytes) const noexcept {
    if (::setsockopt(fd_, SOL_SOCKET, SO_RCVBUF, &buffer_size_bytes, sizeof(buffer_size_bytes)) != 0) {
        return ErrnoStatus("setsockopt(SO_RCVBUF)");
    }
    return y_absl::OkStatus();
}

void NonblockingSocket::TrySetSocketTcpUserTimeout(const PosixTcpOptions& options) const noexcept {
#ifdef TCP_USER_TIMEOUT
    if (const auto timeout_ms = ServerTcpUserTimeoutMs(options)) {
        static_cast<void>(::setsockopt(fd_, IPPROTO_TCP, TCP_USER_TIMEOUT, &*timeout_ms, sizeof(*timeout_ms)));
    }
#else
    static_cast<void>(options);
#endif
}

y_absl::Status NonblockingSocket::Bind(const ResolvedAddress& addr) const noexcept {
    if (::bind(fd_, addr.address(), addr.size()) != 0) {
        return ErrnoStatus("bind");
    }
    return y_absl::OkStatus();
}

y_absl::Status NonblockingSocket::Listen(int backlog) const noexcept {
    if (::listen(fd_, backlog) != 0) {
        return ErrnoStatus("listen");
    }
    return y_absl::OkStatus();
}

y_absl::StatusOr<NonblockingSocket::ResolvedAddress> NonblockingSocket::LocalAddress() const noexcept {
    return AddressOf(fd_, &::getsockname, "getsockname");
}

y_absl::StatusOr<NonblockingSocket::ResolvedAddress> NonblockingSocket::PeerAddress() const noexcept {
    return AddressOf(fd_, &::getpeername, "getpeername");
}

std::optional<AcceptedSocket> NonblockingSocket::Accept4() const noexcept {
    for (;;) {
        sockaddr_storage peer{};
        socklen_t length = sizeof(peer);
        const int fd = ::accept4(fd_, reinterpret_cast<sockaddr*>(&peer), &length, SOCK_NONBLOCK | SOCK_CLOEXEC);
        if (fd >= 0) {
            return AcceptedSocket{
                NonblockingSocket{fd},
                ResolvedAddress{reinterpret_cast<const sockaddr*>(&peer), length}
            };
        }
        if (errno != EINTR) {
            return std::nullopt;
        }
    }
}

std::optional<std::size_t> NonblockingSocket::Recv(std::span<const iovec> buffers) const {
    const int fd = CheckedFd(fd_);
    buffers = TrimBuffers(buffers);
    if (buffers.empty()) {
        return 0;
    }
    return NoblockIo([&] { return ::readv(fd, buffers.data(), static_cast<int>(buffers.size())); }, "Reading a socket");
}

std::optional<std::size_t> NonblockingSocket::Send(std::span<const iovec> buffers) const {
    const int fd = CheckedFd(fd_);
    buffers = TrimBuffers(buffers);
    if (buffers.empty()) {
        return 0;
    }
    try {
        return NoblockIo(
            [&] { return ::writev(fd, buffers.data(), static_cast<int>(buffers.size())); },
            "Writing a socket"
        );
    } catch (const engine::io::IoSystemError& error) {
        if (error.Code().value() == ENOBUFS) {
            return std::nullopt;
        }
        throw;
    }
}

}  // namespace ugrpc::impl::event_engine

USERVER_NAMESPACE_END

#endif
