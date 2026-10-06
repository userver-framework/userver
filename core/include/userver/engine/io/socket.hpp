#pragma once

/// @file userver/engine/io/socket.hpp
/// @brief @copybrief engine::io::Socket

#include <sys/socket.h>

#include <cstddef>
#include <optional>
#include <span>

#include <userver/engine/deadline.hpp>
#include <userver/engine/io/common.hpp>
#include <userver/engine/io/exception.hpp>
#include <userver/engine/io/fd_control_holder.hpp>
#include <userver/engine/io/sockaddr.hpp>

struct iovec;

USERVER_NAMESPACE_BEGIN

namespace engine::ev {
class ThreadControl;
}  // namespace engine::ev

namespace engine::io {

/// Socket type
enum class SocketType {
    kStream = SOCK_STREAM,  ///< Stream socket (e.g. TCP)
    kDgram = SOCK_DGRAM,    ///< Datagram socket (e.g. UDP)

    kTcp = kStream,
    kUdp = kDgram,
};

/// @brief Socket representation.
///
/// It is not thread-safe to concurrently read from socket. It is not
/// thread-safe to concurrently write to socket. However it is safe to
/// concurrently read and write into socket:
/// @snippet core/src/engine/io/socket_test.cpp send self concurrent
class [[nodiscard]] Socket final : public RwBase {
public:
    struct RecvFromResult {
        size_t bytes_received{0};
        Sockaddr src_addr;
    };

    /// Constructs an invalid socket.
    Socket() = default;

    /// Constructs a socket for the address domain of specified type.
    Socket(AddrDomain, SocketType);

    /// @brief Adopts an existing socket for specified address domain.
    /// @note File descriptor will be silently forced to nonblocking mode.
    explicit Socket(int fd, AddrDomain domain = AddrDomain::kUnspecified);

    /// Whether the socket is valid.
    explicit operator bool() const { return IsValid(); }

    /// Whether the socket is valid.
    bool IsValid() const override;

    /// @brief Connects the socket to a specified endpoint.
    /// @note Sockaddr domain must match the socket's domain.
    void Connect(const Sockaddr&, Deadline);

    /// @brief Binds the socket to the specified endpoint.
    /// @note Sockaddr domain must match the socket's domain.
    void Bind(const Sockaddr&);

    /// Starts listening for connections on a specified socket (must be bound).
    void Listen(int backlog = SOMAXCONN);

    /// Suspends current task until the socket has data available.
    /// @returns false on timeout or on task cancellations; true otherwise.
    [[nodiscard]] bool WaitReadable(Deadline) override;

    /// Suspends current task until the socket can accept more data.
    /// @returns false on timeout or on task cancellations; true otherwise.
    [[nodiscard]] bool WaitWriteable(Deadline) override;

    /// @brief Receives at least one byte from the socket.
    /// @returns 0 if connection is closed on one side and no data could be
    /// received any more, received bytes count otherwise.
    [[nodiscard]] size_t RecvSome(void* buf, size_t len, Deadline deadline);

    /// @brief Receives exactly len bytes from the socket.
    /// @note Can return less than len if socket is closed by peer.
    [[nodiscard]] size_t RecvAll(void* buf, size_t len, Deadline deadline);

    /// @brief Receives up to len bytes from the stream
    /// @returns
    /// - nullopt on data absence
    /// - optional{0} if socket is closed by peer.
    /// - optional{data_bytes_available} otherwise,
    ///    1 <= data_bytes_available <= len
    /// @throws IoException if the socket is closed
    /// @throws IoSystemError on a failed syscall, EINTR is retried transparently
    [[nodiscard]] std::optional<size_t> RecvNoblock(void* buf, size_t len);

    /// @brief Receives into a buffer list from the stream without waiting.
    /// @returns
    /// - nullopt on data absence
    /// - optional{0} if socket is closed by peer, or if the buffers hold no space at all
    /// - optional{data_bytes_available} otherwise
    /// @note The iovec descriptors are not modified. The result counts bytes, not buffers, so
    /// utils::IovIter and utils::Advance should be used to resume from it.
    /// @warning At most IOV_MAX buffers are passed to the kernel per call, the rest are filled
    /// by subsequent calls. Leading empty buffers are skipped and do not count towards that
    /// limit.
    /// @note An empty buffer list never reaches the kernel, so on a datagram socket it does not
    /// consume a datagram.
    /// @throws IoException if the socket is closed
    /// @throws IoSystemError on a failed syscall, EINTR is retried transparently
    [[nodiscard]] std::optional<std::size_t> RecvNoblock(std::span<const struct iovec> list);

    /// @brief Sends up to len bytes to the socket without waiting.
    /// @returns
    /// - nullopt on backpressure
    /// - optional{0} if len is 0
    /// - optional{bytes_sent} otherwise, 1 <= bytes_sent <= len
    /// @throws IoException if the socket is closed
    /// @throws IoSystemError on a failed syscall, EINTR is retried transparently
    [[nodiscard]] std::optional<std::size_t> SendNoblock(const void* buf, std::size_t len);

    /// @brief Sends a buffer list to the socket without waiting.
    /// @returns
    /// - nullopt on backpressure
    /// - optional{0} if the buffers hold no bytes at all
    /// - optional{bytes_sent} otherwise
    /// @note The iovec descriptors are not modified. The result counts bytes, not buffers, so
    /// utils::IovIter and utils::Advance should be used to resume from it.
    /// @warning At most IOV_MAX buffers are passed to the kernel per call, the rest have to be
    /// sent by subsequent calls. Leading empty buffers are skipped and do not count towards
    /// that limit.
    /// @note An empty buffer list never reaches the kernel, so on a datagram socket it does not
    /// send an empty datagram.
    /// @throws IoException if the socket is closed
    /// @throws IoSystemError on a failed syscall, EINTR is retried transparently
    [[nodiscard]] std::optional<std::size_t> SendNoblock(std::span<const struct iovec> list);

    /// @brief Sends a buffer vector to the socket.
    /// @note Can return less than len if socket is closed by peer.
    /// @snippet core/src/engine/io/socket_test.cpp send vector data in socket
    [[nodiscard]] size_t SendAll(std::initializer_list<IoData> list, Deadline deadline);

    [[nodiscard]] size_t SendAll(std::span<const IoData> list, Deadline deadline);

    [[nodiscard]] size_t WriteAll(std::span<const IoData> list, Deadline deadline) override {
        return SendAll(list, deadline);
    }

    /// @brief Sends exactly list_size IoData to the socket.
    /// @note Can return less than len if socket is closed by peer.
    [[nodiscard]] size_t SendAll(const IoData* list, std::size_t list_size, Deadline deadline);

    /// @brief Sends exactly list_size iovec to the socket, has no IOV_MAX limits on list_size
    /// @note Can return less than len if socket is closed by peer.
    [[nodiscard]] size_t SendAll(const struct iovec* list, std::size_t list_size, Deadline deadline);

    /// @brief Sends exactly len bytes to the socket.
    /// @note Can return less than len if socket is closed by peer.
    [[nodiscard]] size_t SendAll(const void* buf, size_t len, Deadline deadline);

    /// @brief Accepts a connection from a listening socket.
    /// @see engine::io::Listen
    [[nodiscard]] Socket Accept(Deadline);

    /// @brief Receives at least one byte from the socket, returning source
    /// address.
    /// @returns 0 in bytes_sent if connection is closed on one side and no data
    /// could be received any more, received bytes count otherwise + source
    /// address.
    [[nodiscard]] RecvFromResult RecvSomeFrom(void* buf, size_t len, Deadline deadline);

    /// @brief Sends exactly len bytes to the specified address via the socket.
    /// @note Can return less than len in bytes_sent if socket is closed by peer.
    /// @note Sockaddr domain must match the socket's domain.
    /// @note Not for SocketType::kStream connections, see `man sendto`.
    [[nodiscard]] size_t SendAllTo(const Sockaddr& dest_addr, const void* buf, size_t len, Deadline deadline);

    /// File descriptor corresponding to this socket.
    int Fd() const;

    /// Address of a remote peer.
    const Sockaddr& Getpeername();

    /// Local socket address.
    const Sockaddr& Getsockname();

    /// Releases file descriptor and invalidates the socket.
    [[nodiscard]] int Release() && noexcept;

    /// @brief Closes and invalidates the socket.
    /// @warning You should not call Close with pending I/O. This may work okay
    /// sometimes but it's loosely predictable.
    void Close();

    /// Retrieves a socket option.
    int GetOption(int layer, int optname) const;

    /// Sets a socket option.
    void SetOption(int layer, int optname, int optval);

    /// Sets a socket option with non-trivial optval.
    void SetOption(int layer, int optname, const void* optval, socklen_t optlen);

    /// @brief Receives at least one byte from the socket.
    /// @returns 0 if connection is closed on one side and no data could be
    /// received any more, received bytes count otherwise.
    [[nodiscard]] size_t ReadSome(void* buf, size_t len, Deadline deadline) override {
        return RecvSome(buf, len, deadline);
    }

    /// @brief Receives exactly len bytes from the socket.
    /// @note Can return less than len if socket is closed by peer.
    [[nodiscard]] size_t ReadAll(void* buf, size_t len, Deadline deadline) override {
        return RecvAll(buf, len, deadline);
    }

    /// @brief Receives up to len bytes from the stream
    /// @returns
    /// - nullopt on data absence
    /// - optional{0} if socket is closed by peer.
    /// - optional{data_bytes_available} otherwise,
    ///    1 <= data_bytes_available <= len
    [[nodiscard]] std::optional<size_t> ReadNoblock(void* buf, size_t len) override { return RecvNoblock(buf, len); }

    /// @brief Writes exactly len bytes to the socket.
    /// @note Can return less than len if socket is closed by peer.
    [[nodiscard]] size_t WriteAll(const void* buf, size_t len, Deadline deadline) override {
        return SendAll(buf, len, deadline);
    }

private:
    AddrDomain domain_{AddrDomain::kUnspecified};

    impl::FdControlHolder fd_control_;
    Sockaddr peername_;
    Sockaddr sockname_;
};

}  // namespace engine::io

USERVER_NAMESPACE_END
