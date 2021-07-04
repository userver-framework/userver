#pragma once

#include <arpa/inet.h>

#include <cstdint>
#include <cstdlib>
#include <stdexcept>
#include <utility>

#include <userver/engine/deadline.hpp>
#include <userver/engine/io/addr.hpp>
#include <userver/engine/io/socket.hpp>

namespace engine::io::util_test {

struct TcpListener {
  TcpListener();

  std::pair<Socket, Socket> MakeSocketPair(Deadline);

  uint16_t port;
  Addr addr;
  Socket socket;
};

}  // namespace engine::io::util_test
