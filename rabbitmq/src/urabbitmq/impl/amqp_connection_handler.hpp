#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>

#include <userver/clients/dns/resolver_fwd.hpp>
#include <userver/engine/single_consumer_event.hpp>
#include <userver/utils/periodic_task.hpp>

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
    AmqpConnectionHandler(
        clients::dns::Resolver& resolver,
        const EndpointInfo& endpoint,
        const AuthSettings& auth_settings,
        std::size_t heartbeat_interval_seconds,
        bool secure,
        statistics::ConnectionStatistics& stats,
        engine::Deadline deadline
    );
    ~AmqpConnectionHandler() override;

    void onProperties(AMQP::Connection* connection, const AMQP::Table& server, AMQP::Table& client) override;
    std::uint16_t onNegotiate(AMQP::Connection* connection, std::uint16_t interval) override;

    void onData(AMQP::Connection* connection, const char* buffer, std::size_t size) override;

    void onError(AMQP::Connection* connection, const char* message) override;

    void onClosed(AMQP::Connection* connection) override;

    void onReady(AMQP::Connection* connection) override;

    void OnConnectionCreated(AmqpConnection* connection, engine::Deadline deadline);
    void OnConnectionDestruction();

    void Invalidate();
    bool IsBroken() const;

    void SetOperationDeadline(engine::Deadline deadline);

    void AccountRead(std::size_t size);
    void AccountWrite(std::size_t size);

    statistics::ConnectionStatistics& GetStatistics();

    const AMQP::Address& GetAddress() const;

private:
    void SendHeartbeat();

    AMQP::Address address_;
    std::unique_ptr<engine::io::RwBase> socket_;
    io::SocketReader reader_;
    utils::PeriodicTask heartbeat_task_;
    AmqpConnection* connection_{nullptr};
    std::atomic<std::uint16_t> negotiated_heartbeat_seconds_{0};
    std::uint16_t configured_heartbeat_seconds_{0};

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
