#include <engine/io/socket.hpp>

#include <fcntl.h>
#include <unistd.h>

#include <cerrno>
#include <string>
#include <system_error>

#include <engine/task/task.hpp>
#include <logging/log.hpp>

#include <engine/task/task_context.hpp>
#include <utils/check_syscall.hpp>

namespace engine {
namespace io {
namespace {

int SetNonblock(int fd) {
  int oldflags = utils::CheckSyscall(::fcntl(fd, F_GETFL),
                                     "getting file status flags, fd=", fd);
  if (!(oldflags & O_NONBLOCK)) {
    utils::CheckSyscall(::fcntl(fd, F_SETFL, oldflags | O_NONBLOCK),
                        "setting file status flags, fd=", fd);
  }
  return fd;
}

enum class IoMode { Some, All };

template <typename IoFunc, typename WaitFunc>
size_t PerformIo(IoFunc io_func, int fd, void* buf, size_t size, IoMode io_mode,
                 WaitFunc wait_func, const char* desc) {
  auto* const begin = static_cast<char*>(buf);
  auto* const end = begin + size;

  auto* pos = begin;

  while (pos < end) {
    auto chunk_size = io_func(fd, pos, end - pos);

    if (chunk_size > 0) {
      pos += chunk_size;
    } else if (!chunk_size) {
      break;
    } else if (errno == EINTR) {
      continue;
    } else if (errno == EWOULDBLOCK || errno == EAGAIN) {
      if (pos != begin && io_mode == IoMode::Some) {
        break;
      }
      wait_func();
      if (current_task::GetCurrentTaskContext()->GetWakeupSource() ==
          impl::TaskContext::WakeupSource::kDeadlineTimer) {
        throw SocketTimeout(/*bytes_transferred =*/pos - begin);
      }
    } else {
      const auto err_value = errno;
      std::system_error ex(
          err_value, std::system_category(),
          std::string("Error during ") + desc + ", fd=" + std::to_string(fd));
      LOG_ERROR() << ex.what();
      if (pos != begin) {
        break;
      }
      throw ex;
    }
  }
  return pos - begin;
}

}  // namespace

SocketError::SocketError() : std::runtime_error("generic socket error") {}

SocketTimeout::SocketTimeout() : SocketTimeout(0) {}

SocketTimeout::SocketTimeout(size_t bytes_transferred)
    : SocketError("socket timed out"), bytes_transferred_(bytes_transferred) {}

Socket::Socket()
    : io_watcher_(current_task::GetEventThread()),
      read_waiters_(std::make_shared<impl::WaitList>()),
      write_waiters_(std::make_shared<impl::WaitList>()) {}

Socket::Socket(int fd) : Socket() {
  SetNonblock(fd);
  io_watcher_.SetFd(fd);
}

Socket::Socket(Socket&& other) noexcept : Socket() { *this = std::move(other); }

Socket& Socket::operator=(Socket&& rhs) noexcept {
  io_watcher_.SetFd(std::move(rhs).Release());
  return *this;
}

bool Socket::IsValid() const { return io_watcher_.HasFd(); }

void Socket::WaitReadable(Deadline deadline) {
  impl::WaitList::Lock lock(*read_waiters_);
  auto caller_ctx = current_task::GetCurrentTaskContext();
  impl::TaskContext::SleepParams sleep_params;
  sleep_params.deadline = std::move(deadline);
  sleep_params.wait_list = read_waiters_;
  sleep_params.exec_after_asleep = [this, &lock, caller_ctx] {
    read_waiters_->Append(lock, caller_ctx);
    lock.Release();
  };
  io_watcher_.ReadAsync([waiters = read_waiters_](std::error_code) {
    impl::WaitList::Lock lock(*waiters);
    waiters->WakeupOne(lock);
  });
  caller_ctx->Sleep(std::move(sleep_params));
}

void Socket::WaitWriteable(Deadline deadline) {
  impl::WaitList::Lock lock(*write_waiters_);
  auto caller_ctx = current_task::GetCurrentTaskContext();
  impl::TaskContext::SleepParams sleep_params;
  sleep_params.deadline = std::move(deadline);
  sleep_params.wait_list = write_waiters_;
  sleep_params.exec_after_asleep = [this, &lock, caller_ctx] {
    write_waiters_->Append(lock, caller_ctx);
    lock.Release();
  };
  io_watcher_.WriteAsync([waiters = write_waiters_](std::error_code) {
    impl::WaitList::Lock lock(*waiters);
    waiters->WakeupOne(lock);
  });
  caller_ctx->Sleep(std::move(sleep_params));
}

size_t Socket::Recv(void* buf, size_t size, Deadline deadline) {
  std::lock_guard<Mutex> lock(read_mutex_);
  return PerformIo(&::read, io_watcher_.GetFd(), buf, size, IoMode::Some,
                   [this, deadline] { WaitReadable(deadline); }, "Recv");
}

size_t Socket::RecvAll(void* buf, size_t size, Deadline deadline) {
  std::lock_guard<Mutex> lock(read_mutex_);
  return PerformIo(&::read, io_watcher_.GetFd(), buf, size, IoMode::All,
                   [this, deadline] { WaitReadable(deadline); }, "RecvAll");
}

size_t Socket::Send(void* buf, size_t size, Deadline deadline) {
  std::lock_guard<Mutex> lock(write_mutex_);
  return PerformIo(&::write, io_watcher_.GetFd(), buf, size, IoMode::All,
                   [this, deadline] { WaitWriteable(deadline); }, "Send");
}

Socket Socket::Accept(Deadline deadline) {
  std::lock_guard<Mutex> lock(read_mutex_);
  for (;;) {
    int fd = ::accept(io_watcher_.GetFd(), nullptr, nullptr);
    if (fd != -1) return Socket(fd);

    switch (errno) {
      case EAGAIN:
#if EAGAIN != EWOULDBLOCK
      case EWOULDBLOCK:
#endif
        WaitReadable(deadline);
        if (current_task::GetCurrentTaskContext()->GetWakeupSource() ==
            impl::TaskContext::WakeupSource::kDeadlineTimer) {
          throw SocketTimeout();
        }
        break;

      case ECONNABORTED:  // DOA connection
      case EINTR:         // signal interrupt
      // TCP/IP network errors
      case ENETDOWN:
      case EPROTO:
      case ENOPROTOOPT:
      case EHOSTDOWN:
      case ENONET:
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

void Socket::Close() {
  io_watcher_.Cancel();
  io_watcher_.CloseFd();
}

int Socket::GetFd() const { return io_watcher_.GetFd(); }

int Socket::Release() && noexcept { return io_watcher_.Release(); }

Socket Listen(Addr addr, int backlog) {
  Socket socket(
      utils::CheckSyscall(::socket(addr.Family(), addr.Type(), addr.Protocol()),
                          "creating socket, addr=", addr));

  const int reuse_port_flag = 1;
  utils::CheckSyscall(::setsockopt(socket.GetFd(), SOL_SOCKET, SO_REUSEPORT,
                                   &reuse_port_flag, sizeof(reuse_port_flag)),
                      "setting SO_REUSEPORT, fd=", socket.GetFd());

  utils::CheckSyscall(::bind(socket.GetFd(), addr.Sockaddr(), addr.Addrlen()),
                      "binding a socket, addr=", addr);
  utils::CheckSyscall(::listen(socket.GetFd(), backlog),
                      "listening on a socket, addr=", addr,
                      ", backlog=", backlog);
  return socket;
}

}  // namespace io
}  // namespace engine
