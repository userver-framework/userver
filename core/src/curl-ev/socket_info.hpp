/**
        curl-ev: wrapper for integrating libcurl with libev applications
        Copyright (c) 2013 Oliver Kuckertz <oliver.kuckertz@mologie.de>
        See COPYING for license information.
*/

#pragma once

#include <memory>

#include <engine/ev/thread.hpp>
#include <engine/ev/watcher/io_watcher.hpp>
#include <logging/logger.hpp>
#include <utils/strerror.hpp>

namespace curl {

class EvSocket : public engine::ev::IoWatcher {
 public:
  EvSocket(engine::ev::ThreadControl& thread_control)
      : engine::ev::IoWatcher(thread_control) {}

  ~EvSocket() {}

  enum class Domain { kIPv4, kIPv6 };
  bool Open(Domain domain) {
    int fd =
        socket(domain == Domain::kIPv6 ? AF_INET6 : AF_INET, SOCK_STREAM, 0);
    if (fd == -1) {
      const auto old_errno = errno;
      LOG_ERROR() << "socket(2) failed with error: "
                  << utils::strerror(old_errno);
      return false;
    }
    SetFd(fd);
    return true;
  }

  void cancel() { Cancel(); }

  int native_handle() { return GetFd(); }
  using native_handle_type = int;
};

// For forward declaration
class socket_type : public EvSocket {
 public:
  using EvSocket::EvSocket;
};

class easy;

struct socket_info : public std::enable_shared_from_this<socket_info> {
  socket_info(easy* _handle, std::unique_ptr<socket_type>&& _socket)
      : handle(_handle),
        socket(std::move(_socket)),
        pending_read_op(false),
        pending_write_op(false),
        monitor_read(false),
        monitor_write(false),
        orig_libcurl_socket(socket ? socket->native_handle() : -1) {}

  bool is_cares_socket() const {
    return socket && socket->native_handle() != orig_libcurl_socket;
  }

  easy* handle;
  std::unique_ptr<socket_type> socket;
  bool pending_read_op;
  bool pending_write_op;
  bool monitor_read;
  bool monitor_write;

  native::curl_socket_t orig_libcurl_socket;
};
}  // namespace curl
