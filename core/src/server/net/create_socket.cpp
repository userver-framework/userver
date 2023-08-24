#include "create_socket.hpp"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>
#include <system_error>

#include <boost/filesystem/operations.hpp>

#include <userver/engine/async.hpp>
#include <userver/engine/io/exception.hpp>
#include <userver/engine/io/socket.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/fs/blocking/read.hpp>
#include <userver/fs/blocking/write.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::net {

namespace {
engine::io::Socket CreateUnixSocket(const std::string& path, int backlog) {
  engine::io::Sockaddr addr;
  auto* sa = addr.As<struct sockaddr_un>();
  sa->sun_family = AF_UNIX;

  if (path.size() >= sizeof(sa->sun_path))
    throw std::runtime_error("unix socket path is too long (" + path + ")");
  if (path.empty())
    throw std::runtime_error(
        "unix socket path is empty, abstract sockets are not supported");
  if (path[0] != '/')
    throw std::runtime_error("unix socket path must be absolute (" + path +
                             ")");
  std::strncpy(sa->sun_path, path.c_str(), sizeof(sa->sun_path));

  /* Use blocking API here, it is not critical as CreateUnixSocket() is called
   * on startup only */

  if (fs::blocking::GetFileType(path) ==
      boost::filesystem::file_type::socket_file)
    fs::blocking::RemoveSingleFile(path);

  engine::io::Socket socket{addr.Domain(), engine::io::SocketType::kStream};
  socket.Bind(addr);
  socket.Listen(backlog);

  auto perms = static_cast<boost::filesystem::perms>(0666);
  fs::blocking::Chmod(path, perms);
  return socket;
}

engine::io::Socket CreateIpv6Socket(uint16_t port, int backlog) {
  engine::io::Sockaddr addr;
  auto* sa = addr.As<struct sockaddr_in6>();
  sa->sin6_family = AF_INET6;
  // may be implemented as a macro
  // NOLINTNEXTLINE(hicpp-no-assembler, readability-isolate-declaration)
  sa->sin6_port = htons(port);
  sa->sin6_addr = in6addr_any;

  engine::io::Socket socket{addr.Domain(), engine::io::SocketType::kStream};
  socket.Bind(addr);
  socket.Listen(backlog);
  return socket;
}

}  // namespace

engine::io::Socket CreateSocket(const ListenerConfig& config) {
  if (config.unix_socket_path.empty())
    return CreateIpv6Socket(config.port, config.backlog);
  else
    return CreateUnixSocket(config.unix_socket_path, config.backlog);
}

}  // namespace server::net

USERVER_NAMESPACE_END
