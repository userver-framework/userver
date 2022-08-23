#include <userver/internal/net/net_listener.hpp>

#include <arpa/inet.h>

#include <userver/engine/async.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace internal::net {
namespace {

auto IpVersionToDomain(IpVersion ipv) {
  switch (ipv) {
    case IpVersion::kV6:
      return engine::io::AddrDomain::kInet6;
    case IpVersion::kV4:
      return engine::io::AddrDomain::kInet;
  }
  UINVARIANT(false, "Unexpected ip version");
}

template <typename Listener>
void ListenerCtor(Listener& listener, IpVersion ipv) {
  in_port_t* port_ptr = nullptr;

  switch (ipv) {
    case IpVersion::kV6: {
      auto* sa = listener.addr.template As<struct sockaddr_in6>();
      sa->sin6_family = AF_INET6;
      sa->sin6_addr = in6addr_loopback;
      port_ptr = &sa->sin6_port;
    } break;
    case IpVersion::kV4: {
      auto* sa = listener.addr.template As<struct sockaddr_in>();
      sa->sin_family = AF_INET;
      // may be implemented as a macro
      // NOLINTNEXTLINE(hicpp-no-assembler, readability-isolate-declaration)
      sa->sin_addr.s_addr = htonl(INADDR_LOOPBACK);
      port_ptr = &sa->sin_port;
    } break;
  }

  UASSERT(port_ptr);
  int attempts = 100;
  while (attempts--) {
    // NOLINTNEXTLINE(cert-msc50-cpp, concurrency-mt-unsafe)
    listener.port = 1024 + (rand() % (65536 - 1024));
    // may be implemented as a macro
    // NOLINTNEXTLINE(hicpp-no-assembler, readability-isolate-declaration)
    *port_ptr = htons(listener.port);

    try {
      listener.socket.Bind(listener.addr);
      return;
    } catch (const engine::io::IoException&) {
      // retry
    }
  }
  throw std::runtime_error("Could not find a port to listen");
}

}  // namespace

TcpListener::TcpListener(IpVersion ipv) : socket{IpVersionToDomain(ipv), type} {
  ListenerCtor(*this, ipv);
  socket.Listen();
}

std::pair<engine::io::Socket, engine::io::Socket> TcpListener::MakeSocketPair(
    engine::Deadline deadline) {
  auto connect_task = engine::AsyncNoSpan([this, deadline] {
    engine::io::Socket peer_socket{addr.Domain(),
                                   engine::io::SocketType::kStream};
    peer_socket.Connect(addr, deadline);
    return peer_socket;
  });
  std::pair<engine::io::Socket, engine::io::Socket> socket_pair;
  socket_pair.first = socket.Accept(deadline);
  socket_pair.second = connect_task.Get();
  return socket_pair;
}

UdpListener::UdpListener(IpVersion ipv) : socket{IpVersionToDomain(ipv), type} {
  ListenerCtor(*this, ipv);
}

}  // namespace internal::net

USERVER_NAMESPACE_END
