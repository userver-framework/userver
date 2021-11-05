#include <engine/io/util_test.hpp>

#include <userver/engine/async.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::io::util_test {
namespace {

template <typename Listener>
void ListenerCtor(Listener& listener) {
  auto* sa = listener.addr.template As<struct sockaddr_in6>();
  sa->sin6_family = AF_INET6;
  sa->sin6_addr = in6addr_loopback;

  int attempts = 100;
  while (attempts--) {
    listener.port = 1024 + (rand() % (65536 - 1024));
    sa->sin6_port = htons(listener.port);

    try {
      listener.socket.Bind(listener.addr);
      return;
    } catch (const io::IoException&) {
      // retry
    }
  }
  throw std::runtime_error("Could not find a port to listen");
}

}  // namespace

TcpListener::TcpListener() {
  ListenerCtor(*this);
  socket.Listen();
}

std::pair<Socket, Socket> TcpListener::MakeSocketPair(Deadline deadline) {
  auto connect_task = engine::AsyncNoSpan([this, deadline] {
    Socket peer_socket{addr.Domain(), SocketType::kStream};
    peer_socket.Connect(addr, deadline);
    return peer_socket;
  });
  std::pair<Socket, Socket> socket_pair;
  socket_pair.first = socket.Accept(deadline);
  socket_pair.second = connect_task.Get();
  return socket_pair;
}

UdpListener::UdpListener() { ListenerCtor(*this); }

}  // namespace engine::io::util_test

USERVER_NAMESPACE_END
