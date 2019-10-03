#pragma once

/// @file engine/io/socket.hpp
/// @brief @copybrief engine::io::Socket

#include <engine/deadline.hpp>
#include <engine/io/addr.hpp>
#include <engine/io/common.hpp>
#include <engine/io/exception.hpp>
#include <engine/io/fd_control_holder.hpp>
#include <utils/clang_format_workarounds.hpp>

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
  bool WaitReadable(Deadline) override;

  /// Suspends current task until the socket can accept more data.
  bool WaitWriteable(Deadline);

  /// Receives at least one byte from the socket.
  [[nodiscard]] size_t RecvSome(void* buf, size_t len, Deadline deadline);

  /// @brief Receives exactly len bytes from the socket.
  /// @note Can return less than len if socket is closed by peer.
  [[nodiscard]] size_t RecvAll(void* buf, size_t len, Deadline deadline);

  /// @brief Sends exactly len bytes to the socket.
  /// @note Can return less than len if socket is closed by peer.
  [[nodiscard]] size_t SendAll(const void* buf, size_t len, Deadline deadline);

  /// @brief Accepts a connection from a listening socket.
  /// @see engine::io::Listen
  Socket Accept(Deadline);

  /// File descriptor corresponding to this socket.
  int Fd() const;

  /// Address of a remote peer.
  const Addr& Getpeername();

  /// Local socket address.
  const Addr& Getsockname();

  /// Releases file descriptor and invalidates the socket.
  [[nodiscard]] int Release() && noexcept;

  /// @brief Closes and invalidates the socket.
  /// @warn You should not call Close with pending I/O. This may work okay
  /// sometimes but it's loosely predictable.
  void Close();

  /// Retrieves a socket option.
  int GetOption(int layer, int optname) const;

  /// Sets a socket option.
  void SetOption(int layer, int optname, int optval);

  /// Receives at least one byte from the socket.
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
  impl::FdControlHolder fd_control_;
  Addr peername_;
  Addr sockname_;
};

/// Connects to a remote peer specified by addr.
Socket Connect(Addr addr, Deadline deadline);

/// Creates a listening socket bound to addr.
Socket Listen(Addr addr, int backlog = SOMAXCONN);

}  // namespace engine::io
