#include "unix_socket_sink.hpp"

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <iostream>

#include <spdlog/spdlog.h>

#include <utils/check_syscall.hpp>
#include <utils/strerror.hpp>

USERVER_NAMESPACE_BEGIN

namespace logging::impl {

void UnixSocketClient::connect(const spdlog::filename_t& filename) {
  struct sockaddr_un addr {};
  addr.sun_family = AF_UNIX;
  std::strncpy(addr.sun_path, filename.data(), filename.length());

  socket_ =
      utils::CheckSyscall(::socket(AF_UNIX, SOCK_STREAM, 0), "create socket");

  utils::CheckSyscall(
      ::connect(socket_, reinterpret_cast<const struct sockaddr*>(&addr),
                sizeof(addr)),
      "connect to server by unix-socket");
}

void UnixSocketClient::send(const char* data, size_t n_bytes) {
  size_t bytes_sent = 0;
  while (bytes_sent < n_bytes) {
    auto write_res =
        ::send(socket_, data + bytes_sent, n_bytes - bytes_sent, 0);

    if (write_res < 0) {
      const auto old_errno = errno;
      close();
      errno = old_errno;
      utils::CheckSyscall(write_res, "write to socket");
    }

    bytes_sent += static_cast<size_t>(write_res);
  }
}

void UnixSocketClient::close() {
  if (socket_ != -1) {
    if (::close(socket_) == -1) {
      std::string error_msg = utils::strerror(errno);
      std::cerr << "Error while closing socket: " << error_msg << std::endl;
    }

    socket_ = -1;
  }
}

UnixSocketClient::~UnixSocketClient() { close(); }

}  // namespace logging::impl

USERVER_NAMESPACE_END
