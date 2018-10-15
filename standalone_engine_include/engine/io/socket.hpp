#pragma once

#include <memory>

#include <engine/deadline.hpp>
#include <engine/mutex.hpp>

#include <engine/ev/watcher/io_watcher.hpp>
#include <engine/wait_list.hpp>
#include "addr.hpp"

namespace engine {
namespace io {

class SocketError : public std::runtime_error {
 public:
  SocketError();
  using std::runtime_error::runtime_error;
};

class SocketTimeout : public SocketError {
 public:
  SocketTimeout();
  explicit SocketTimeout(size_t bytes_transferred);

  size_t BytesTransferred() const;

 private:
  size_t bytes_transferred_;
};

class Socket {
 public:
  Socket();

  // fd will be silently forced to nonblocking mode
  explicit Socket(int fd);

  Socket(const Socket&) = delete;
  Socket(Socket&&) noexcept;

  Socket& operator=(const Socket&) = delete;
  Socket& operator=(Socket&&) noexcept;

  explicit operator bool() const { return IsValid(); }
  bool IsValid() const;

  void WaitReadable(Deadline);
  void WaitWriteable(Deadline);

  size_t Recv(void* buf, size_t size, Deadline deadline);
  size_t RecvAll(void* buf, size_t size, Deadline deadline);
  size_t Send(void* buf, size_t size, Deadline deadline);

  Socket Accept(Deadline);

  int GetFd() const;
  __attribute__((warn_unused_result)) int Release() && noexcept;

  void Close();

 private:
  ev::IoWatcher io_watcher_;
  Mutex read_mutex_;
  std::shared_ptr<impl::WaitList> read_waiters_;
  Mutex write_mutex_;
  std::shared_ptr<impl::WaitList> write_waiters_;
};

Socket Listen(Addr addr, int backlog = SOMAXCONN);

}  // namespace io
}  // namespace engine
