#include <userver/engine/io/socket.hpp>

#include <fcntl.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <unistd.h>

#include <cerrno>
#include <string>
#include <vector>

#include <userver/engine/io/exception.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/logging/log.hpp>
#include <userver/utils/assert.hpp>

#include <build_config.hpp>
#include <engine/io/fd_control.hpp>
#include <utils/check_syscall.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::io {
namespace {

constexpr size_t kMaxStackSizeVector = 32;

// MAC_COMPAT: does not accept flags in type
impl::FdControlHolder MakeSocket(AddrDomain domain, SocketType type) {
  return impl::FdControl::Adopt(
      utils::CheckSyscallCustomException<IoSystemError>(
          ::socket(static_cast<int>(domain),
#ifdef SOCK_NONBLOCK
                   SOCK_NONBLOCK |
#endif
#ifdef SOCK_CLOEXEC
                       SOCK_CLOEXEC |
#endif
                       static_cast<int>(type),
                   /* protocol=*/0),
          "creating socket"));
}

template <typename Format, typename... Args>
Sockaddr& MemoizeAddr(Sockaddr& addr, decltype(&::getpeername) getter,
                      const Socket& socket, const Format& format,
                      const Args&... args) {
  if (addr.Domain() == AddrDomain::kUnspecified) {
    auto len = addr.Capacity();
    utils::CheckSyscallCustomException<IoSystemError>(
        getter(socket.Fd(), addr.Data(), &len), format, args...);
    UASSERT(len <= addr.Capacity());
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
    socklen_t addrlen = src_addr_.Capacity();
    const auto ret = ::recvfrom(fd, buf, len, 0, src_addr_.Data(), &addrlen);
    if (ret != -1 && addrlen > src_addr_.Capacity()) {
      throw IoException()
          << "Peer address does not fit into AddrStorage, family="
          << src_addr_.Data()->sa_family << ", addrlen=" << addrlen;
    }
    return ret;
  }

  const Sockaddr& SourceAddress() { return src_addr_; }

 private:
  Sockaddr src_addr_;
};

class SendToWrapper {
 public:
  SendToWrapper(const Sockaddr& dest_addr) : dest_addr_(dest_addr) {}

  [[nodiscard]] ssize_t operator()(int fd, const void* buf, size_t len) const {
    return ::sendto(fd, buf, len,
// MAC_COMPAT: does not support MSG_NOSIGNAL
#ifdef MSG_NOSIGNAL
                    MSG_NOSIGNAL |
#endif
                        0,
                    dest_addr_.Data(), dest_addr_.Size());
  }

 private:
  const Sockaddr& dest_addr_;
};

void FillIoSendData(const IoData* data, struct iovec* dst, std::size_t count) {
  UASSERT(data);
  UASSERT(count > 0);
  for (size_t i = 0; i < count; ++i) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
    dst[i].iov_base = const_cast<void*>(data[i].data);
    dst[i].iov_len = data[i].len;
  }
}

}  // namespace

Socket::Socket(AddrDomain domain, SocketType type)
    : domain_(domain), fd_control_(MakeSocket(domain, type)) {}

Socket::Socket(int fd, AddrDomain domain)
    : domain_(domain), fd_control_(impl::FdControl::Adopt(fd)) {
// MAC_COMPAT: no socket domain access on mac
#ifdef SO_DOMAIN
  if (domain_ != AddrDomain::kUnspecified) {
    const int adopted_domain = GetOption(SOL_SOCKET, SO_DOMAIN);
    if (static_cast<int>(domain_) != adopted_domain) {
      throw AddrException(fmt::format(
          "Adopted socket domain ({}) does not match the provided one ({})",
          adopted_domain, static_cast<int>(domain_)));
    }
  }
#endif
}

bool Socket::IsValid() const { return !!fd_control_; }

void Socket::Connect(const Sockaddr& addr, Deadline deadline) {
  UASSERT(IsValid());

  if (addr.Domain() != domain_) {
    throw AddrException(fmt::format(
        "Socket address domain ({}) does not match address domain ({})",
        static_cast<int>(domain_), static_cast<int>(addr.Domain())));
  }

  peername_ = addr;

  if (!::connect(Fd(), addr.Data(), addr.Size())) {
    return;
  }

  int err_value = errno;
  if (err_value == EINPROGRESS) {
    if (!WaitWriteable(deadline)) {
      if (current_task::ShouldCancel()) {
        throw IoCancelled() << "Connect to " << addr;
      }
      throw IoTimeout() << "Connect to " << addr;
    }
    err_value = GetOption(SOL_SOCKET, SO_ERROR);
  }

  if (err_value) {
    throw IoSystemError(err_value, "Socket")
        << "Error while establishing connection, fd=" << Fd()
        << ", addr=" << addr;
  }
}

void Socket::Bind(const Sockaddr& addr) {
  UASSERT(IsValid());

  if (addr.Domain() != domain_) {
    throw AddrException(fmt::format(
        "Socket address domain ({}) does not match address domain ({})",
        static_cast<int>(domain_), static_cast<int>(addr.Domain())));
  }

  SetOption(SOL_SOCKET, SO_REUSEADDR, 1);

// MAC_COMPAT: does not support REUSEPORT
#ifdef SO_REUSEPORT
  SetOption(SOL_SOCKET, SO_REUSEPORT, 1);
#else
  LOG_ERROR() << "SO_REUSEPORT is not defined, you may experience problems "
                 "with multithreaded listeners";
#endif

  utils::CheckSyscallCustomException<IoSystemError>(
      ::bind(Fd(), addr.Data(), addr.Size()),
      "binding a socket, fd={}, addr={}", Fd(), addr);
}

// NOLINTNEXTLINE(readability-make-member-function-const)
void Socket::Listen(int backlog) {
  UASSERT(IsValid());

  utils::CheckSyscallCustomException<IoSystemError>(
      ::listen(Fd(), backlog), "listening on a socket, fd={}, backlog={}", Fd(),
      backlog);
}

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
  impl::Direction::SingleUserGuard guard(dir);
  return dir.PerformIo(guard, &RecvWrapper, buf, len, impl::TransferMode::kOnce,
                       deadline, "RecvSome from ", peername_);
}

size_t Socket::RecvAll(void* buf, size_t len, Deadline deadline) {
  if (!IsValid()) {
    throw IoException("Attempt to RecvAll from closed socket");
  }
  auto& dir = fd_control_->Read();
  impl::Direction::SingleUserGuard guard(dir);
  return dir.PerformIo(guard, &RecvWrapper, buf, len,
                       impl::TransferMode::kWhole, deadline, "RecvAll from ",
                       peername_);
}

size_t Socket::SendAll(std::initializer_list<IoData> list, Deadline deadline) {
  return SendAll(list.begin(), list.size(), deadline);
}

size_t Socket::SendAll(const IoData* list, std::size_t list_size,
                       Deadline deadline) {
  if (list_size < kMaxStackSizeVector) {
    /// stack
    std::array<struct ::iovec, kMaxStackSizeVector> data{};
    FillIoSendData(list, data.data(), list_size);
    return SendAll(data.data(), list_size, deadline);
  } else {
    /// heap
    std::vector<struct ::iovec> data(list_size);
    FillIoSendData(list, data.data(), list_size);
    return SendAll(data.data(), list_size, deadline);
  }
}

size_t Socket::SendAll(const struct iovec* list, std::size_t list_size,
                       Deadline deadline) {
  if (!IsValid()) {
    throw IoException("Attempt to SendAll to closed socket");
  }
  UASSERT(list);
  UASSERT(list_size > 0);
  UINVARIANT(list_size <= IOV_MAX, "To big array of IoData for SendAll");
  auto& dir = fd_control_->Write();
  impl::Direction::SingleUserGuard guard(dir);
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
  return dir.PerformIoV(guard, &writev, const_cast<struct iovec*>(list),
                        list_size, impl::TransferMode::kWhole, deadline,
                        "SendAll to ", peername_);
}

size_t Socket::SendAll(const void* buf, size_t len, Deadline deadline) {
  if (!IsValid()) {
    throw IoException("Attempt to SendAll to closed socket");
  }
  auto& dir = fd_control_->Write();
  impl::Direction::SingleUserGuard guard(dir);
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
  return dir.PerformIo(guard, &SendWrapper, const_cast<void*>(buf), len,
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
    impl::Direction::SingleUserGuard guard(dir);
    result.bytes_received =
        dir.PerformIo(guard, recv_from_wrapper, buf, len,
                      impl::TransferMode::kOnce, deadline, "RecvSomeFrom");
  }
  result.src_addr = recv_from_wrapper.SourceAddress();
  return result;
}

size_t Socket::SendAllTo(const Sockaddr& dest_addr, const void* buf, size_t len,
                         Deadline deadline) {
  if (!IsValid()) {
    throw IoException("Attempt to SendAll to closed socket");
  }
  if (dest_addr.Domain() != domain_) {
    throw AddrException(fmt::format(
        "Socket address domain ({}) does not match address domain ({})",
        static_cast<int>(domain_), static_cast<int>(dest_addr.Domain())));
  }

  auto& dir = fd_control_->Write();
  impl::Direction::SingleUserGuard guard(dir);
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
  return dir.PerformIo(guard, SendToWrapper{dest_addr}, const_cast<void*>(buf),
                       len, impl::TransferMode::kWhole, deadline,
                       "SendAllTo to ", dest_addr);
}

Socket Socket::Accept(Deadline deadline) {
  if (!IsValid()) {
    throw IoException("Attempt to Accept from closed socket");
  }
  auto& dir = fd_control_->Read();
  impl::Direction::SingleUserGuard guard(dir);
  for (;;) {
    Sockaddr buf;
    auto len = buf.Capacity();

// MAC_COMPAT: no accept4
#ifdef HAVE_ACCEPT4
    int fd =
        ::accept4(dir.Fd(), buf.Data(), &len, SOCK_NONBLOCK | SOCK_CLOEXEC);
#else
    int fd = ::accept(dir.Fd(), buf.Data(), &len);
#endif

    UASSERT(len <= buf.Capacity());
    if (fd != -1) {
      auto peersock = Socket(fd);
      peersock.peername_ = buf;
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
          throw IoTimeout() << "Accept";
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
        utils::CheckSyscallCustomException<IoSystemError>(
            -1, "accepting a connection");
    }
  }
}

int Socket::Fd() const { return fd_control_ ? fd_control_->Fd() : kInvalidFd; }

const Sockaddr& Socket::Getpeername() {
  UASSERT(IsValid());
  return MemoizeAddr(peername_, &::getpeername, *this,
                     "getting peer name, fd={}", Fd());
}

const Sockaddr& Socket::Getsockname() {
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
  utils::CheckSyscallCustomException<IoSystemError>(
      ::getsockopt(Fd(), layer, optname, &value, &value_len),
      "getting socket option {},{} on fd {}", layer, optname, Fd());
  UASSERT(value_len == sizeof(value));
  return value;
}

// NOLINTNEXTLINE(readability-make-member-function-const)
void Socket::SetOption(int layer, int optname, int optval) {
  UASSERT(IsValid());
  utils::CheckSyscallCustomException<IoSystemError>(
      ::setsockopt(Fd(), layer, optname, &optval, sizeof(optval)),
      "setting socket option {},{} to {} on fd {}", layer, optname, optval,
      Fd());
}

}  // namespace engine::io

USERVER_NAMESPACE_END
