#pragma once

#include <memory>
#include <optional>
#include <stdexcept>
#include <string>

#include <userver/clients/dns/resolver_fwd.hpp>
#include <userver/engine/single_consumer_event.hpp>

#include <urabbitmq/impl/io/socket_reader.hpp>

#include <amqpcpp.h>

USERVER_NAMESPACE_BEGIN

namespace engine::io {
class RwBase;
}

namespace urabbitmq {

struct EndpointInfo;
struct AuthSettings;

namespace statistics {
class ConnectionStatistics;
}

namespace impl {

class AmqpConnection;

class ConnectionSetupError final : public std::runtime_error {
  using std::runtime_error::runtime_error;
};

class ConnectionSetupTimeout final : public std::runtime_error {
  using std::runtime_error::runtime_error;
};

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

  void OnConnectionCreated(AmqpConnection* connection,
                           engine::Deadline deadline);
  void OnConnectionDestruction();

  void Invalidate();
  bool IsBroken() const;

  void SetOperationDeadline(engine::Deadline deadline);

  void AccountRead(size_t size);
  void AccountWrite(size_t size);

  statistics::ConnectionStatistics& GetStatistics();

  const AMQP::Address& GetAddress() const;

 private:
  AMQP::Address address_;
  std::unique_ptr<engine::io::RwBase> socket_;
  io::SocketReader reader_;

  engine::SingleConsumerEvent connection_ready_event_;
  std::atomic<bool> broken_{false};

  statistics::ConnectionStatistics& stats_;

  engine::Deadline operation_deadline_ = engine::Deadline::Passed();

  std::atomic<bool> is_ready_{false};
  std::optional<std::string> error_;
};

}  // namespace impl
}  // namespace urabbitmq

USERVER_NAMESPACE_END
