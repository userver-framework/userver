#pragma once

#include <engine/ev/thread_control.hpp>
#include <userver/clients/dns/resolver_fwd.hpp>
#include <userver/engine/io/socket.hpp>

#include <urabbitmq/impl/io/socket_reader.hpp>
#include <urabbitmq/impl/io/socket_writer.hpp>

#include <amqpcpp.h>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq::impl {

class AmqpConnectionHandler final : public AMQP::ConnectionHandler {
 public:
  AmqpConnectionHandler(clients::dns::Resolver& resolver,
                        engine::ev::ThreadControl& thread,
                        const AMQP::Address& address);
  ~AmqpConnectionHandler() override;

  engine::ev::ThreadControl& GetEvThread();

  void onData(AMQP::Connection* connection, const char* buffer,
              size_t size) override;

  void onError(AMQP::Connection* connection, const char* message) override;

  void onClosed(AMQP::Connection* connection) override;

  void OnConnectionCreated(AMQP::Connection* connection);
  void OnConnectionDestruction();

  void Invalidate();
  bool IsBroken() const;

 private:
  engine::ev::ThreadControl thread_;

  engine::io::Socket socket_;
  io::SocketWriter writer_;
  io::SocketReader reader_;

  std::atomic<bool> broken_{false};
};

}  // namespace urabbitmq::impl

USERVER_NAMESPACE_END