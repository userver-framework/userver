#include <userver/engine/io/socket.hpp>

#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <string>

#include <userver/engine/io/exception.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/logging/log.hpp>
#include <userver/utils/assert.hpp>

#include <build_config.hpp>
#include <engine/io/fd_control.hpp>
#include <utils/check_syscall.hpp>

namespace engine::io {
namespace {

// MAC_COMPAT: does not accept flags in type
Socket MakeSocket(const Addr& addr) {
  return Socket(utils::CheckSyscall(::socket(addr.Family(),
#ifdef SOCK_NONBLOCK
                                             SOCK_NONBLOCK |
#endif
#ifdef SOCK_CLOEXEC
                                                 SOCK_CLOEXEC |
#endif
                                                 addr.Type(),
                                             addr.Protocol()),
                                    "creating socket, addr={}", addr));
}

template <typename Format, typename... Args>
Addr& MemoizeAddr(Addr& addr, decltype(&::getpeername) getter,
                  const Socket& socket, const Format& format,
                  const Args&... args) {
  if (addr.Domain() == AddrDomain::kInvalid) {
    AddrStorage buf;
    auto len = buf.Size();
    utils::CheckSyscall(getter(socket.Fd(), buf.Data(), &len), format, args...);
    UASSERT(len <= buf.Size());
    addr = Addr(buf, 0, 0);
  }
  return addr;
}

// IoFunc wrappers for Direction::PerformIo

[[nodiscard]] ssize_t RecvWrapper(int fd, void* buf, size_t len) {
  return ::recv(fd, buf, len, 0);
}

[[nodiscard]] ssize_t SendWrapper(int fd, const void* buf, size_t len) {
  return ::send(fd, buf, len,
// MAC_COMPAT: does not support MSG_NOSIGNAL
#ifdef MSG_NOSIGNAL
                MSG_NOSIGNAL |
#endif
                    0);
}

class RecvFromWrapper {
 public:
  [[nodiscard]] ssize_t operator()(int fd, void* buf, size_t len) {
    socklen_t addrlen = src_addr_.Size();
    const auto ret = ::recvfrom(fd, buf, len, 0, src_addr_.Data(), &addrlen);
    if (ret != -1 && addrlen > src_addr_.Size()) {
      throw IoException()
          << "Peer address does not fit into AddrStorage, family="
          << src_addr_.Data()->sa_family << ", addrlen=" << addrlen;
    }
    return ret;
  }

  const AddrStorage& SourceAddress() { return src_addr_; }

 private:
  AddrStorage src_addr_;
};

class SendToWrapper {
 public:
  SendToWrapper(const Addr& dest_addr) : dest_addr_(dest_addr) {}

  [[nodiscard]] ssize_t operator()(int fd, const void* buf, size_t len) const {
    return ::sendto(fd, buf, len,
// MAC_COMPAT: does not support MSG_NOSIGNAL
#ifdef MSG_NOSIGNAL
                    MSG_NOSIGNAL |
#endif
                        0,
                    dest_addr_.Sockaddr(), dest_addr_.Addrlen());
  }

 private:
  const Addr& dest_addr_;
};

}  // namespace

ConnectTimeout::ConnectTimeout()
    : IoException("connection establishment timed out") {}

Socket::Socket(int fd) : fd_control_(impl::FdControl::Adopt(fd)) {}

bool Socket::IsValid() const { return !!fd_control_; }

bool Socket::WaitReadable(Deadline deadline) {
  UASSERT(IsValid());
  return fd_control_->Read().Wait(deadline);
}

bool Socket::WaitWriteable(Deadline deadline) {
  UASSERT(IsValid());
  return fd_control_->Write().Wait(deadline);
}

size_t Socket::RecvSome(void* buf, size_t len, Deadline deadline) {
  if (!IsValid()) {
    throw IoException("Attempt to RecvSome from closed socket");
  }
  auto& dir = fd_control_->Read();
  impl::Direction::Lock lock(dir);
  return dir.PerformIo(lock, &RecvWrapper, buf, len,
                       impl::TransferMode::kPartial, deadline, "RecvSome from ",
                       peername_);
}

size_t Socket::RecvAll(void* buf, size_t len, Deadline deadline) {
  if (!IsValid()) {
    throw IoException("Attempt to RecvAll from closed socket");
  }
  auto& dir = fd_control_->Read();
  impl::Direction::Lock lock(dir);
  return dir.PerformIo(lock, &RecvWrapper, buf, len, impl::TransferMode::kWhole,
                       deadline, "RecvAll from ", peername_);
}

size_t Socket::SendAll(const void* buf, size_t len, Deadline deadline) {
  if (!IsValid()) {
    throw IoException("Attempt to SendAll to closed socket");
  }
  auto& dir = fd_control_->Write();
  impl::Direction::Lock lock(dir);
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
  return dir.PerformIo(lock, &SendWrapper, const_cast<void*>(buf), len,
                       impl::TransferMode::kWhole, deadline, "SendAll to ",
                       peername_);
}

Socket::RecvFromResult Socket::RecvSomeFrom(void* buf, size_t len,
                                            Deadline deadline) {
  if (!IsValid()) {
    throw IoException("Attempt to RecvSomeFrom via closed socket");
  }
  RecvFromResult result;
  RecvFromWrapper recv_from_wrapper;
  {
    auto& dir = fd_control_->Read();
    impl::Direction::Lock lock(dir);
    result.bytes_received =
        dir.PerformIo(lock, recv_from_wrapper, buf, len,
                      impl::TransferMode::kPartial, deadline, "RecvSomeFrom");
  }
  result.src_addr = {recv_from_wrapper.SourceAddress(), 0, 0};
  return result;
}

size_t Socket::SendAllTo(const Addr& dest_addr, const void* buf, size_t len,
                         Deadline deadline) {
  if (!IsValid()) {
    throw IoException("Attempt to SendAll to closed socket");
  }
  auto& dir = fd_control_->Write();
  impl::Direction::Lock lock(dir);
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
  return dir.PerformIo(lock, SendToWrapper{dest_addr}, const_cast<void*>(buf),
                       len, impl::TransferMode::kWhole, deadline,
                       "SendAllTo to ", dest_addr);
}

Socket Socket::Accept(Deadline deadline) {
  if (!IsValid()) {
    throw IoException("Attempt to Accept from closed socket");
  }
  auto& dir = fd_control_->Read();
  impl::Direction::Lock lock(dir);
  for (;;) {
    AddrStorage buf;
    auto len = buf.Size();

// MAC_COMPAT: no accept4
#ifdef HAVE_ACCEPT4
    int fd =
        ::accept4(dir.Fd(), buf.Data(), &len, SOCK_NONBLOCK | SOCK_CLOEXEC);
#else
    int fd = ::accept(dir.Fd(), buf.Data(), &len);
#endif

    UASSERT(len <= buf.Size());
    if (fd != -1) {
      auto peersock = Socket(fd);
      peersock.peername_ = Addr(buf, 0, 0);
      return peersock;
    }

    switch (errno) {
      case EAGAIN:
#if EAGAIN != EWOULDBLOCK
      case EWOULDBLOCK:
#endif
        if (!WaitReadable(deadline)) {
          if (current_task::ShouldCancel()) {
            throw IoCancelled() << "Accept";
          }
          throw ConnectTimeout();
        }
        break;

      case ECONNABORTED:  // DOA connection
      case EINTR:         // signal interrupt
      // TCP/IP network errors
      case ENETDOWN:
      case EPROTO:
      case ENOPROTOOPT:
      case EHOSTDOWN:
#ifdef ENONET  // No ENONET in Mac OS
      case ENONET:
#endif
      case EHOSTUNREACH:
      case EOPNOTSUPP:
      case ENETUNREACH:
        // retry accept()
        break;

      default:
        utils::CheckSyscall(-1, "accepting a connection");
    }
  }
}

int Socket::Fd() const { return fd_control_ ? fd_control_->Fd() : kInvalidFd; }

const Addr& Socket::Getpeername() {
  UASSERT(IsValid());
  return MemoizeAddr(peername_, &::getpeername, *this,
                     "getting peer name, fd={}", Fd());
}

const Addr& Socket::Getsockname() {
  UASSERT(IsValid());
  return MemoizeAddr(sockname_, &::getsockname, *this,
                     "getting socket name, fd={}", Fd());
}

int Socket::Release() && noexcept {
  const int fd = Fd();
  if (IsValid()) {
    fd_control_->Invalidate();
    fd_control_.reset();
  }
  return fd;
}

void Socket::Close() { fd_control_.reset(); }

int Socket::GetOption(int layer, int optname) const {
  UASSERT(IsValid());
  int value = -1;
  socklen_t value_len = sizeof(value);
  utils::CheckSyscall(::getsockopt(Fd(), layer, optname, &value, &value_len),
                      "getting socket option {},{} on fd {}", layer, optname,
                      Fd());
  UASSERT(value_len == sizeof(value));
  return value;
}

void Socket::SetOption(int layer, int optname, int optval) {
  UASSERT(IsValid());
  utils::CheckSyscall(
      ::setsockopt(Fd(), layer, optname, &optval, sizeof(optval)),
      "setting socket option {},{} to {} on fd {}", layer, optname, optval,
      Fd());
}

Socket Connect(const Addr& addr, Deadline deadline) {
  auto socket = MakeSocket(addr);
  socket.peername_ = addr;

  int err_value = ::connect(socket.Fd(), addr.Sockaddr(), addr.Addrlen());
  if (!err_value) {
    return socket;
  }
  err_value = errno;
  if (err_value == EINPROGRESS) {
    if (!socket.WaitWriteable(deadline)) {
      if (current_task::ShouldCancel()) {
        throw IoCancelled() << "Connect";
      }
      throw ConnectTimeout();
    }
    err_value = socket.GetOption(SOL_SOCKET, SO_ERROR);
  }

  if (err_value) {
    throw IoSystemError(err_value)
        << "Error while establishing connection, fd=" << socket.Fd()
        << ", addr=" << addr;
  }
  return socket;
}

Socket Listen(const Addr& addr, int backlog) {
  auto socket = Bind(addr);
  utils::CheckSyscall(::listen(socket.Fd(), backlog),
                      "listening on a socket, addr={}, backlog={}", addr,
                      backlog);
  return socket;
}

Socket Bind(const Addr& addr) {
  auto socket = MakeSocket(addr);

  socket.SetOption(SOL_SOCKET, SO_REUSEADDR, 1);

// MAC_COMPAT: does not support REUSEPORT
#ifdef SO_REUSEPORT
  socket.SetOption(SOL_SOCKET, SO_REUSEPORT, 1);
#else
  LOG_ERROR() << "SO_REUSEPORT is not defined, you may experience problems "
                 "with multithreaded listeners";
#endif

  utils::CheckSyscall(::bind(socket.Fd(), addr.Sockaddr(), addr.Addrlen()),
                      "binding a socket, addr={}", addr);
  return socket;
}

}  // namespace engine::io
