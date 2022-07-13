#include "amqp_connection_handler.hpp"

#include <netinet/in.h>
#include <netinet/tcp.h>

#include <userver/clients/dns/resolver.hpp>
#include <userver/urabbitmq/client_settings.hpp>

#include <engine/ev/thread_control.hpp>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq::impl {

namespace {

engine::io::Socket CreateSocket(engine::io::Sockaddr& addr) {
  engine::io::Socket socket{addr.Domain(), engine::io::SocketType::kTcp};
  socket.SetOption(IPPROTO_TCP, TCP_NODELAY, 1);
  socket.Connect(addr, {});

  return socket;
}

engine::io::Socket CreateSocket(clients::dns::Resolver& resolver,
                                const AMQP::Address& address) {
  auto addrs = resolver.Resolve(address.hostname(), {});
  for (auto& addr : addrs) {
    addr.SetPort(static_cast<int>(address.port()));
    try {
      return CreateSocket(addr);
    } catch (const std::exception&) {
    }
  }

  throw std::runtime_error{
      "couldn't connect to to any of the resolved addresses"};
}

std::unique_ptr<io::ISocket> CreateWrappedSocket(
    clients::dns::Resolver& resolver, const AMQP::Address& address) {
  auto socket = CreateSocket(resolver, address);

  const bool secure = address.secure();
  if (secure) {
    // TODO : https://github.com/userver-framework/userver/issues/52
    // This might end up in busy loop if remote RMQ crashes
    return std::make_unique<io::SecureSocket>(std::move(socket));
  } else {
    return std::make_unique<io::NonSecureSocket>(std::move(socket));
  }
}

namespace {

AMQP::Address ToAmqpAddress(const EndpointInfo& endpoint,
                            const AuthSettings& settings) {
  return {endpoint.host, endpoint.port,
          AMQP::Login{settings.login, settings.password}, settings.vhost,
          settings.secure};
}

}  // namespace

}  // namespace

AmqpConnectionHandler::AmqpConnectionHandler(clients::dns::Resolver& resolver,
                                             engine::ev::ThreadControl& thread,
                                             const EndpointInfo& endpoint,
                                             const AuthSettings& auth_settings)
    : thread_{thread},
      socket_{CreateWrappedSocket(resolver,
                                  ToAmqpAddress(endpoint, auth_settings))},
      writer_{*this, thread_, *socket_},
      reader_{*this, thread_, *socket_} {}

AmqpConnectionHandler::~AmqpConnectionHandler() {
  writer_.Stop();
  reader_.Stop();
}

engine::ev::ThreadControl& AmqpConnectionHandler::GetEvThread() {
  return thread_;
}

void AmqpConnectionHandler::onData(AMQP::Connection* connection,
                                   const char* buffer, size_t size) {
  UASSERT(thread_.IsInEvThread());

  writer_.Write(connection, buffer, size);
}

void AmqpConnectionHandler::onError(AMQP::Connection*, const char*) {
  Invalidate();
}

void AmqpConnectionHandler::onClosed(AMQP::Connection*) { Invalidate(); }

void AmqpConnectionHandler::OnConnectionCreated(AMQP::Connection* connection) {
  reader_.Start(connection);
}

void AmqpConnectionHandler::OnConnectionDestruction() {
  writer_.Stop();
  reader_.Stop();
}

void AmqpConnectionHandler::Invalidate() { broken_ = true; }

bool AmqpConnectionHandler::IsBroken() const { return broken_; }

}  // namespace urabbitmq::impl

USERVER_NAMESPACE_END