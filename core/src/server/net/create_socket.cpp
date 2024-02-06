#include "create_socket.hpp"

#include <string>

#include <fmt/format.h>
#include <boost/filesystem/operations.hpp>

#include <userver/engine/io/socket.hpp>
#include <userver/fs/blocking/read.hpp>
#include <userver/fs/blocking/write.hpp>
#include <userver/net/blocking/get_addr_info.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::net {

namespace {

engine::io::Socket CreateUnixSocket(const std::string& path, int backlog) {
  const auto addr = engine::io::Sockaddr::MakeUnixSocketAddress(path);

  /* Use blocking API here, it is not critical as CreateUnixSocket() is called
   * on startup only */

  if (fs::blocking::GetFileType(path) ==
      boost::filesystem::file_type::socket_file)
    fs::blocking::RemoveSingleFile(path);

  engine::io::Socket socket{addr.Domain(), engine::io::SocketType::kStream};
  socket.Bind(addr);
  socket.Listen(backlog);

  constexpr auto perms = static_cast<boost::filesystem::perms>(0666);
  fs::blocking::Chmod(path, perms);
  return socket;
}

engine::io::Socket CreateIpv6Socket(const std::string& address, uint16_t port,
                                    int backlog) {
  std::vector<engine::io::Sockaddr> addrs;

  try {
    addrs = USERVER_NAMESPACE::net::blocking::GetAddrInfo(
        address, std::to_string(port).c_str());
  } catch (const std::runtime_error&) {
    throw std::runtime_error(
        fmt::format("Address string '{}' is invalid", address));
  }

  UASSERT(!addrs.empty());

  if (addrs.size() > 1)
    throw std::runtime_error(fmt::format(
        "Address string '{}' designates multiple addresses, while only 1 "
        "address per listener is supported. The addresses are: {}\nYou can "
        "specify '::' as address to listen on all local addresses",
        fmt::join(addrs, ", "), address));

  auto& addr = addrs.front();
  engine::io::Socket socket{addr.Domain(), engine::io::SocketType::kStream};
  socket.Bind(addr);
  socket.Listen(backlog);
  return socket;
}

}  // namespace

engine::io::Socket CreateSocket(const ListenerConfig& config) {
  if (config.unix_socket_path.empty())
    return CreateIpv6Socket(config.address, config.port, config.backlog);
  else
    return CreateUnixSocket(config.unix_socket_path, config.backlog);
}

}  // namespace server::net

USERVER_NAMESPACE_END
