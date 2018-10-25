#pragma once

#include <engine/deadline.hpp>
#include <engine/io/addr.hpp>
#include <engine/io/error.hpp>

#include <engine/io/fd_control.hpp>

namespace engine {
namespace io {

namespace impl {
class FdControl;
}  // namespace impl

class ConnectTimeout : public IoError {
 public:
  ConnectTimeout();
};

// I/O methods (Wait*, Recv*, Send*, Accept) and const methods are coro-safe,
// all others are NOT coro-safe.
// You may call Close with pending I/O though, but this is not recommended
// in general (with active I/O it can cause loosely predictable effects).
class Socket {
 public:
  // returned from closed socket
  static constexpr int kInvalidFd = -1;

  Socket() = default;

  // fd will be silently forced to nonblocking mode
  explicit Socket(int fd);

  explicit operator bool() const { return IsOpen(); }
  bool IsOpen() const;

  void WaitReadable(Deadline);
  void WaitWriteable(Deadline);

  size_t RecvSome(void* buf, size_t len, Deadline deadline);
  size_t RecvAll(void* buf, size_t len, Deadline deadline);
  size_t SendAll(const void* buf, size_t len, Deadline deadline);

  Socket Accept(Deadline);

  int Fd() const;
  const Addr& Getpeername();
  const Addr& Getsockname();

  __attribute__((warn_unused_result)) int Release() && noexcept;

  void Close();

  int GetOption(int layer, int optname) const;
  void SetOption(int layer, int optname, int optval);

 private:
  impl::FdControlHolder fd_control_;
  Addr peername_;
  Addr sockname_;
};

// Creates stream sockets
Socket Connect(Addr addr, Deadline deadline);
Socket Listen(Addr addr, int backlog = SOMAXCONN);

}  // namespace io
}  // namespace engine
