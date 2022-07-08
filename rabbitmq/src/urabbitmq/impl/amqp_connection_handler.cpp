#include "amqp_connection_handler.hpp"

#include <netinet/in.h>
#include <netinet/tcp.h>

#include <engine/ev/thread_control.hpp>

#include <userver/clients/dns/resolver.hpp>

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

}  // namespace

AmqpConnectionHandler::AmqpConnectionHandler(clients::dns::Resolver& resolver,
                                             engine::ev::ThreadControl& thread,
                                             const AMQP::Address& address)
    : thread_{thread},
      socket_{CreateSocket(resolver, address)},
      writer_{*this, thread_, socket_.Fd()},
      reader_{*this, thread_, socket_.Fd()} {}

AmqpConnectionHandler::~AmqpConnectionHandler() {
  writer_.Stop();
  reader_.Stop();
  socket_.Close();
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