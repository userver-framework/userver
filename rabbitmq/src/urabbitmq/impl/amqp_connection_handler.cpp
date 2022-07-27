#include "amqp_connection_handler.hpp"

#include <netinet/in.h>
#include <netinet/tcp.h>

#include <userver/clients/dns/resolver.hpp>
#include <userver/urabbitmq/client_settings.hpp>

#include <engine/ev/thread_control.hpp>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq::impl {

constexpr std::chrono::milliseconds kSocketConnectTimeout{2000};

namespace {

engine::io::Socket CreateSocket(engine::io::Sockaddr& addr,
                                engine::Deadline deadline) {
  engine::io::Socket socket{addr.Domain(), engine::io::SocketType::kTcp};
  socket.SetOption(IPPROTO_TCP, TCP_NODELAY, 1);
  socket.Connect(addr, deadline);

  return socket;
}

engine::io::Socket CreateSocket(clients::dns::Resolver& resolver,
                                const AMQP::Address& address,
                                engine::Deadline deadline) {
  auto addrs = resolver.Resolve(address.hostname(), {});
  for (auto& addr : addrs) {
    addr.SetPort(static_cast<int>(address.port()));
    try {
      return CreateSocket(addr, deadline);
    } catch (const std::exception&) {
    }
  }

  throw std::runtime_error{
      "couldn't connect to to any of the resolved addresses"};
}

std::unique_ptr<io::ISocket> CreateSocketPtr(clients::dns::Resolver& resolver,
                                             const AMQP::Address& address,
                                             engine::Deadline deadline) {
  auto socket = CreateSocket(resolver, address, deadline);

  const bool secure = address.secure();
  if (secure) {
    // TODO : https://github.com/userver-framework/userver/issues/52
    // This might end up in busy loop if remote RMQ crashes
    return std::make_unique<io::SecureSocket>(std::move(socket), deadline);
  } else {
    return std::make_unique<io::NonSecureSocket>(std::move(socket));
  }
}

AMQP::Address ToAmqpAddress(const EndpointInfo& endpoint,
                            const AuthSettings& settings) {
  return {endpoint.host, endpoint.port,
          AMQP::Login{settings.login, settings.password}, settings.vhost,
          settings.secure};
}

}  // namespace

AmqpConnectionHandler::AmqpConnectionHandler(clients::dns::Resolver& resolver,
                                             engine::ev::ThreadControl& thread,
                                             const EndpointInfo& endpoint,
                                             const AuthSettings& auth_settings)
    : thread_{thread},
      socket_{CreateSocketPtr(
          resolver, ToAmqpAddress(endpoint, auth_settings),
          engine::Deadline::FromDuration(kSocketConnectTimeout))},
      writer_{*this, *socket_},
      reader_{*this, *socket_} {}

AmqpConnectionHandler::~AmqpConnectionHandler() {
  writer_.Stop();
  reader_.Stop();
}

engine::ev::ThreadControl& AmqpConnectionHandler::GetEvThread() {
  return thread_;
}

void AmqpConnectionHandler::onProperties(AMQP::Connection*, const AMQP::Table&,
                                         AMQP::Table& client) {
  client["product"] = "uServer AMQP library";
  client["copyright"] = "Copyright 2022-2022 Yandex NV";
  client["information"] = "TODO : link to docs";
}

void AmqpConnectionHandler::onData(AMQP::Connection* connection,
                                   const char* buffer, size_t size) {
  UASSERT(thread_.IsInEvThread());

  writer_.Write(connection, buffer, size);
  flow_control_.AccountWrite(size);
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

void AmqpConnectionHandler::AccountBufferFlush(size_t size) {
  flow_control_.AccountFlush(size);
}

bool AmqpConnectionHandler::IsWriteable() {
  return !broken_.load(std::memory_order_relaxed) && !flow_control_.IsBlocked();
}

void AmqpConnectionHandler::WriteBufferFlowControl::AccountWrite(size_t size) {
  buffer_size_ += size;
  if (buffer_size_ > kFlowControlStartThreshold) {
    blocked_.store(true, std::memory_order_relaxed);
  }
}

void AmqpConnectionHandler::WriteBufferFlowControl::AccountFlush(size_t size) {
  buffer_size_ -= size;
  if (buffer_size_ < kFlowControlStopThreshold) {
    blocked_.store(false, std::memory_order_relaxed);
  }
}

bool AmqpConnectionHandler::WriteBufferFlowControl::IsBlocked() const {
  return blocked_.load(std::memory_order_relaxed);
}

}  // namespace urabbitmq::impl

USERVER_NAMESPACE_END