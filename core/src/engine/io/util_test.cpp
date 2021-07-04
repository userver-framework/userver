#include <engine/io/util_test.hpp>

#include <userver/engine/async.hpp>

namespace engine::io::util_test {

TcpListener::TcpListener() : port(0) {
  io::AddrStorage addr_storage;
  auto* sa = addr_storage.As<struct sockaddr_in6>();
  sa->sin6_family = AF_INET6;
  sa->sin6_addr = in6addr_loopback;

  int attempts = 100;
  while (attempts--) {
    port = 1024 + (rand() % (65536 - 1024));
    sa->sin6_port = htons(port);
    addr = io::Addr(addr_storage, SOCK_STREAM, 0);

    try {
      socket = io::Listen(addr);
      return;
    } catch (const io::IoException&) {
      // retry
    }
  }
  throw std::runtime_error("Could not find a port to listen");
}

std::pair<Socket, Socket> TcpListener::MakeSocketPair(Deadline deadline) {
  auto connect_task =
      engine::impl::Async([&] { return io::Connect(addr, deadline); });
  std::pair<Socket, Socket> socket_pair;
  socket_pair.first = socket.Accept(deadline);
  socket_pair.second = connect_task.Get();
  return socket_pair;
}

}  // namespace engine::io::util_test
