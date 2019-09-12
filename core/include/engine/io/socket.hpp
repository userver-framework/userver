#pragma once

#include <engine/deadline.hpp>
#include <engine/io/addr.hpp>
#include <engine/io/error.hpp>
#include <engine/io/fd_control_holder.hpp>
#include <utils/clang_format_workarounds.hpp>

namespace engine::io {

namespace impl {
class FdControl;
}  // namespace impl

/// Socket connection timeout.
class ConnectTimeout : public IoError {
 public:
  ConnectTimeout();
};

/// Socket representation.
class USERVER_NODISCARD Socket final {
 public:
  /// File descriptor of an invalid socket.
  static constexpr int kInvalidFd = -1;

  /// Constructs an invalid socket.
  Socket() = default;

  /// Adopts an existing socket.
  /// @note File descriptor will be silently forced to nonblocking mode.
  explicit Socket(int fd);

  /// Whether the socket is valid.
  /// @{
  explicit operator bool() const { return IsValid(); }
  bool IsValid() const;
  /// @}

  /// Suspends current task until the socket has data available.
  bool WaitReadable(Deadline);

  /// Suspends current task until the socket can accept more data.
  bool WaitWriteable(Deadline);

  /// Receives at least one bytes from the socket.
  [[nodiscard]] size_t RecvSome(void* buf, size_t len, Deadline deadline);

  /// Receives exactly len bytes from the socket.
  /// @note Can return less than len if socket is closed by peer.
  [[nodiscard]] size_t RecvAll(void* buf, size_t len, Deadline deadline);

  /// Sends exactly len bytes to the socket.
  /// @note Can return less than len if socket is closed by peer.
  [[nodiscard]] size_t SendAll(const void* buf, size_t len, Deadline deadline);

  /// Accepts a connection from a listening socket.
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

  /// Closes and invalidates the socket.
  /// @warn You should not call Close with pending I/O. This may work okay
  /// sometimes but it's loosely predictable.
  void Close();

  /// Retrieves a socket option.
  int GetOption(int layer, int optname) const;

  /// Sets a socket option.
  void SetOption(int layer, int optname, int optval);

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
