#include <engine/io/util_test.hpp>

#include <userver/engine/async.hpp>

namespace engine::io::util_test {
namespace {

template <typename Listener, typename SocketFunc>
void ListenerCtor(Listener& listener, int socket_type,
                  SocketFunc&& socket_func) {
  io::AddrStorage addr_storage;
  auto* sa = addr_storage.As<struct sockaddr_in6>();
  sa->sin6_family = AF_INET6;
  sa->sin6_addr = in6addr_loopback;

  int attempts = 100;
  while (attempts--) {
    listener.port = 1024 + (rand() % (65536 - 1024));
    sa->sin6_port = htons(listener.port);
    listener.addr = io::Addr(addr_storage, socket_type, 0);

    try {
      listener.socket = socket_func(listener.addr);
      return;
    } catch (const io::IoException&) {
      // retry
    }
  }
  throw std::runtime_error("Could not find a port to listen");
}

}  // namespace

TcpListener::TcpListener() {
  ListenerCtor(*this, SOCK_STREAM,
               [](const Addr& addr) { return io::Listen(addr); });
}

std::pair<Socket, Socket> TcpListener::MakeSocketPair(Deadline deadline) {
  auto connect_task =
      engine::impl::Async([&] { return io::Connect(addr, deadline); });
  std::pair<Socket, Socket> socket_pair;
  socket_pair.first = socket.Accept(deadline);
  socket_pair.second = connect_task.Get();
  return socket_pair;
}

UdpListener::UdpListener() { ListenerCtor(*this, SOCK_DGRAM, &io::Bind); }

}  // namespace engine::io::util_test
