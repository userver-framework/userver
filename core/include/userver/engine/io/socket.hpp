#pragma once

/// @file userver/engine/io/socket.hpp
/// @brief @copybrief engine::io::Socket

#include <userver/engine/deadline.hpp>
#include <userver/engine/io/addr.hpp>
#include <userver/engine/io/common.hpp>
#include <userver/engine/io/exception.hpp>
#include <userver/engine/io/fd_control_holder.hpp>
#include <userver/utils/clang_format_workarounds.hpp>

namespace engine::io {

namespace impl {
class FdControl;
}  // namespace impl

/// Socket connection timeout.
class ConnectTimeout : public IoException {
 public:
  ConnectTimeout();
};

/// Socket representation.
class USERVER_NODISCARD Socket final : public ReadableBase {
 public:
  struct RecvFromResult {
    size_t bytes_received{0};
    Addr src_addr;
  };

  /// Constructs an invalid socket.
  Socket() = default;

  /// @brief Adopts an existing socket.
  /// @note File descriptor will be silently forced to nonblocking mode.
  explicit Socket(int fd);

  /// Whether the socket is valid.
  explicit operator bool() const { return IsValid(); }

  /// Whether the socket is valid.
  bool IsValid() const override;

  /// Suspends current task until the socket has data available.
  [[nodiscard]] bool WaitReadable(Deadline) override;

  /// Suspends current task until the socket can accept more data.
  [[nodiscard]] bool WaitWriteable(Deadline);

  /// @brief Receives at least one byte from the socket.
  /// @returns 0 if connnection is closed on one side and no data could be
  /// received any more, received bytes count otherwise.
  [[nodiscard]] size_t RecvSome(void* buf, size_t len, Deadline deadline);

  /// @brief Receives exactly len bytes from the socket.
  /// @note Can return less than len if socket is closed by peer.
  [[nodiscard]] size_t RecvAll(void* buf, size_t len, Deadline deadline);

  /// @brief Sends exactly len bytes to the socket.
  /// @note Can return less than len if socket is closed by peer.
  [[nodiscard]] size_t SendAll(const void* buf, size_t len, Deadline deadline);

  /// @brief Accepts a connection from a listening socket.
  /// @see engine::io::Listen
  [[nodiscard]] Socket Accept(Deadline);

  /// @brief Receives at least one byte from the socket, returning source
  /// address.
  /// @returns 0 in bytes_sent if connnection is closed on one side and no data
  /// could be received any more, received bytes count otherwise + source
  /// address.
  [[nodiscard]] RecvFromResult RecvSomeFrom(void* buf, size_t len,
                                            Deadline deadline);

  /// @brief Sends exactly len bytes to the specified address via the socket.
  /// @note Can return less than len in bytes_sent if socket is closed by peer.
  /// @note Not for SOCK_STREAM connections, see `man sendto`.
  [[nodiscard]] size_t SendAllTo(const Addr& dest_addr, const void* buf,
                                 size_t len, Deadline deadline);

  /// File descriptor corresponding to this socket.
  int Fd() const;

  /// Address of a remote peer.
  const Addr& Getpeername();

  /// Local socket address.
  const Addr& Getsockname();

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

  /// @brief Receives at least one byte from the socket.
  /// @returns 0 if connnection is closed on one side and no data could be
  /// received any more, received bytes count otherwise.
  [[nodiscard]] size_t ReadSome(void* buf, size_t len,
                                Deadline deadline) override {
    return RecvSome(buf, len, deadline);
  }

  /// @brief Receives exactly len bytes from the socket.
  /// @note Can return less than len if socket is closed by peer.
  [[nodiscard]] size_t ReadAll(void* buf, size_t len,
                               Deadline deadline) override {
    return RecvAll(buf, len, deadline);
  }

 private:
  // NOTE: should become a nonstatic member function if connectionless protocols
  // support is added, safer this way for now.
  friend Socket Connect(const Addr&, Deadline);

  impl::FdControlHolder fd_control_;
  Addr peername_;
  Addr sockname_;
};

/// Connects to a remote peer specified by addr.
[[nodiscard]] Socket Connect(const Addr& addr, Deadline deadline);

/// Creates a listening socket bound to addr.
[[nodiscard]] Socket Listen(const Addr& addr, int backlog = SOMAXCONN);

/// Creates a socket bound to addr
[[nodiscard]] Socket Bind(const Addr& addr);

}  // namespace engine::io
