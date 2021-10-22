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

namespace engine::io::util_test {

struct TcpListener {
  TcpListener();

  std::pair<Socket, Socket> MakeSocketPair(Deadline);

  constexpr static auto type = SocketType::kStream;

  uint16_t port{0};
  Sockaddr addr;
  Socket socket{AddrDomain::kInet6, type};
};

struct UdpListener {
  UdpListener();

  constexpr static auto type = SocketType::kDgram;

  uint16_t port{0};
  Sockaddr addr;
  Socket socket{AddrDomain::kInet6, type};
};

}  // namespace engine::io::util_test

USERVER_NAMESPACE_END
