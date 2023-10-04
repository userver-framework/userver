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
  constexpr static auto kType = engine::io::SocketType::kStream;

  explicit TcpListener(IpVersion ipv = IpVersion::kV6);

  std::pair<engine::io::Socket, engine::io::Socket> MakeSocketPair(
      engine::Deadline);

  std::uint16_t Port() const { return addr.Port(); }

  engine::io::Sockaddr addr;
  engine::io::Socket socket;
};

struct UdpListener {
  constexpr static auto kType = engine::io::SocketType::kDgram;

  explicit UdpListener(IpVersion ipv = IpVersion::kV6);

  std::uint16_t Port() const { return addr.Port(); }

  engine::io::Sockaddr addr;
  engine::io::Socket socket;
};

}  // namespace internal::net

USERVER_NAMESPACE_END
