#pragma once

#include <memory>

#include <userver/clients/dns/resolver_fwd.hpp>
#include <userver/engine/io/socket.hpp>
#include <userver/engine/single_consumer_event.hpp>

#include <urabbitmq/impl/io/isocket.hpp>
#include <urabbitmq/impl/io/socket_reader.hpp>

#include <amqpcpp.h>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq {

struct EndpointInfo;
struct AuthSettings;

namespace statistics {
class ConnectionStatistics;
}

namespace impl {

class AmqpConnection;

class AmqpConnectionHandler final : public AMQP::ConnectionHandler {
 public:
  AmqpConnectionHandler(clients::dns::Resolver& resolver,
                        const EndpointInfo& endpoint,
                        const AuthSettings& auth_settings, bool secure,
                        statistics::ConnectionStatistics& stats,
                        engine::Deadline deadline);
  ~AmqpConnectionHandler() override;

  void onProperties(AMQP::Connection* connection, const AMQP::Table& server,
                    AMQP::Table& client) override;

  void onData(AMQP::Connection* connection, const char* buffer,
              size_t size) override;

  void onError(AMQP::Connection* connection, const char* message) override;

  void onClosed(AMQP::Connection* connection) override;

  void onReady(AMQP::Connection* connection) override;

  void OnConnectionCreated(AmqpConnection* connection, engine::Deadline deadline);
  void OnConnectionDestruction();

  void Invalidate();
  bool IsBroken() const;

  void AccountRead(size_t size);

  void SetOperationDeadline(engine::Deadline deadline);

  statistics::ConnectionStatistics& GetStatistics();

 private:
  std::unique_ptr<io::ISocket> socket_;
  io::SocketReader reader_;

  engine::SingleConsumerEvent connection_ready_event_;
  std::atomic<bool> broken_{false};

  statistics::ConnectionStatistics& stats_;

  engine::Deadline operation_deadline_ = engine::Deadline::Passed();
};

}  // namespace impl
}  // namespace urabbitmq

USERVER_NAMESPACE_END