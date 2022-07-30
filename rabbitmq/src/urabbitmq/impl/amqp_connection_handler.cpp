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
    return std::make_unique<io::SecureSocket>(std::move(socket), deadline);
  } else {
    return std::make_unique<io::NonSecureSocket>(std::move(socket));
  }
}

AMQP::Address ToAmqpAddress(const EndpointInfo& endpoint,
                            const AuthSettings& settings, bool secure) {
  return {endpoint.host, endpoint.port,
          AMQP::Login{settings.login, settings.password}, settings.vhost,
          secure};
}

}  // namespace

AmqpConnectionHandler::AmqpConnectionHandler(clients::dns::Resolver& resolver,
                                             engine::ev::ThreadControl& thread,
                                             const EndpointInfo& endpoint,
                                             const AuthSettings& auth_settings,
                                             bool secure)
    : thread_{thread},
      socket_{CreateSocketPtr(
          resolver, ToAmqpAddress(endpoint, auth_settings, secure),
          engine::Deadline::FromDuration(kSocketConnectTimeout))},
      writer_{*this, *socket_},
      reader_{*this, *socket_},
      state_{std::make_shared<HandlerState>()},
      flow_control_{*state_} {}

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

void AmqpConnectionHandler::Invalidate() { state_->SetBroken(); }

bool AmqpConnectionHandler::IsBroken() const { return state_->IsBroken(); }

void AmqpConnectionHandler::AccountBufferFlush(size_t size) {
  flow_control_.AccountFlush(size);
}

std::shared_ptr<HandlerState> AmqpConnectionHandler::GetState() const {
  return state_;
}

AmqpConnectionHandler::WriteBufferFlowControl::WriteBufferFlowControl(
    HandlerState& state)
    : state_{state} {}

void AmqpConnectionHandler::WriteBufferFlowControl::AccountWrite(size_t size) {
  buffer_size_ += size;
  if (buffer_size_ > kFlowControlStartThreshold) {
    state_.SetBlocked();
  }
}

void AmqpConnectionHandler::WriteBufferFlowControl::AccountFlush(size_t size) {
  buffer_size_ -= size;
  if (buffer_size_ < kFlowControlStopThreshold) {
    state_.SetUnblocked();
  }
}

}  // namespace urabbitmq::impl

USERVER_NAMESPACE_END