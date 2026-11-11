#pragma once

#if defined(ARCADIA_ROOT) && defined(__linux__)

#include <y_absl/status/status.h>
#include <y_absl/status/statusor.h>

#include <cstddef>
#include <optional>
#include <span>
#include <utility>

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/uio.h>

#include <grpc/event_engine/event_engine.h>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::impl::event_engine {

struct PosixTcpOptions;
struct AcceptedSocket;

class [[nodiscard]] NonblockingSocket final {
public:
    using ResolvedAddress = grpc_event_engine::experimental::EventEngine::ResolvedAddress;

    NonblockingSocket() = default;
    explicit NonblockingSocket(int fd) noexcept;
    NonblockingSocket(NonblockingSocket&& other) noexcept : fd_(std::exchange(other.fd_, kInvalidFd)) {}
    NonblockingSocket& operator=(NonblockingSocket&& other) noexcept;
    NonblockingSocket(const NonblockingSocket&) = delete;
    NonblockingSocket& operator=(const NonblockingSocket&) = delete;
    ~NonblockingSocket() { Close(); }

    static std::optional<NonblockingSocket> Create(int family) noexcept;

    [[nodiscard]] bool IsValid() const noexcept { return fd_ >= 0; }
    [[nodiscard]] int Fd() const noexcept { return fd_; }
    [[nodiscard]] int Release() && noexcept { return std::exchange(fd_, kInvalidFd); }
    void Close() noexcept;

    y_absl::Status SetSocketLowLatency(int low_latency) const noexcept;
    y_absl::Status SetSocketReuseAddr(int reuse) const noexcept;
    y_absl::Status SetSocketReusePort(int reuse) const noexcept;
    bool SetSocketDualStack() const noexcept;
    y_absl::Status SetSocketRcvBuf(int buffer_size_bytes) const noexcept;
    void TrySetSocketTcpUserTimeout(const PosixTcpOptions& options) const noexcept;
    y_absl::Status Bind(const ResolvedAddress& addr) const noexcept;
    y_absl::Status Listen(int backlog) const noexcept;
    y_absl::StatusOr<ResolvedAddress> LocalAddress() const noexcept;
    y_absl::StatusOr<ResolvedAddress> PeerAddress() const noexcept;
    std::optional<AcceptedSocket> Accept4() const noexcept;

    [[nodiscard]] std::optional<std::size_t> Recv(std::span<const iovec> iov) const;

    [[nodiscard]] std::optional<std::size_t> Send(std::span<const iovec> iov) const;

private:
    static constexpr int kInvalidFd = -1;
    int fd_{kInvalidFd};
};

struct AcceptedSocket final {
    NonblockingSocket sock;
    NonblockingSocket::ResolvedAddress addr;
};

}  // namespace ugrpc::impl::event_engine

USERVER_NAMESPACE_END

#endif
