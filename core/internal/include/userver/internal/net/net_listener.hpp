#pragma once

#include <arpa/inet.h>

#include <cstdint>
#include <cstdlib>
#include <stdexcept>
#include <utility>

#include <userver/engine/deadline.hpp>
#include <userver/engine/io/sockaddr.hpp>
#include <userver/engine/io/socket.hpp>

USERVER_NAMESPACE_BEGIN

namespace internal::net {

enum class IpVersion {
  kV6,
  kV4,
};

struct TcpListener {
  explicit TcpListener(IpVersion ipv = IpVersion::kV6);

  std::pair<engine::io::Socket, engine::io::Socket> MakeSocketPair(
      engine::Deadline);

  constexpr static auto type = engine::io::SocketType::kStream;

  uint16_t port{0};
  engine::io::Sockaddr addr;
  engine::io::Socket socket;
};

struct UdpListener {
  explicit UdpListener(IpVersion ipv = IpVersion::kV6);

  constexpr static auto type = engine::io::SocketType::kDgram;

  uint16_t port{0};
  engine::io::Sockaddr addr;
  engine::io::Socket socket;
};

}  // namespace internal::net

USERVER_NAMESPACE_END
