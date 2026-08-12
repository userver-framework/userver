#include "amqp_connection_handler.hpp"

#include <algorithm>
#include <limits>

#include <netinet/in.h>
#include <netinet/tcp.h>

#include <userver/clients/dns/resolver.hpp>
#include <userver/engine/io/common.hpp>
#include <userver/engine/io/socket.hpp>
#include <userver/engine/io/tls_wrapper.hpp>
#include <userver/logging/log.hpp>
#include <userver/urabbitmq/client_settings.hpp>
#include <userver/utils/assert.hpp>

#include <urabbitmq/impl/amqp_connection.hpp>
#include <urabbitmq/statistics/connection_statistics.hpp>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq::impl {

namespace {

engine::io::Socket CreateSocket(engine::io::Sockaddr& addr, engine::Deadline deadline) {
    engine::io::Socket socket{addr.Domain(), engine::io::SocketType::kTcp};
    socket.SetOption(IPPROTO_TCP, TCP_NODELAY, 1);
    socket.Connect(addr, deadline);

    return socket;
}

engine::io::Socket CreateSocket(
    clients::dns::Resolver& resolver,
    const AMQP::Address& address,
    engine::Deadline deadline
) {
    auto addrs = resolver.Resolve(address.hostname(), {});
    for (auto& addr : addrs) {
        addr.SetPort(static_cast<int>(address.port()));
        try {
            return CreateSocket(addr, deadline);
        } catch (const std::exception&) {
        }
    }

    throw std::runtime_error{"Couldn't connect to any of the resolved addresses"};
}

std::unique_ptr<engine::io::RwBase> CreateSocketPtr(
    clients::dns::Resolver& resolver,
    const AMQP::Address& address,
    const AuthSettings& auth_settings,
    engine::Deadline deadline
) {
    auto socket = CreateSocket(resolver, address, deadline);

    const bool secure = address.secure();
    if (secure) {
        if (auth_settings.tls_settings.has_value()) {
            const auto& tls_settings = *auth_settings.tls_settings;
            const crypto::Certificate& client_cert =
                tls_settings.client_cert_settings ? tls_settings.client_cert_settings->cert : crypto::Certificate();
            const crypto::PrivateKey& client_key =
                tls_settings.client_cert_settings ? tls_settings.client_cert_settings->key : crypto::PrivateKey();

            return std::make_unique<engine::io::TlsWrapper>(engine::io::TlsWrapper::StartTlsClient(
                std::move(socket),
                tls_settings.verify_host ? address.hostname() : "",
                client_cert,
                client_key,
                deadline,
                tls_settings.ca_certs
            ));
        }

        return std::make_unique<engine::io::TlsWrapper>(engine::io::TlsWrapper::StartTlsClient(
            std::move(socket),
            address.hostname(),
            deadline
        ));  // verify_host is true by default
    } else {
        return std::make_unique<engine::io::Socket>(std::move(socket));
    }
}

AMQP::Address ToAmqpAddress(const EndpointInfo& endpoint, const AuthSettings& settings, bool secure) {
    return {endpoint.host, endpoint.port, AMQP::Login{settings.login, settings.password}, settings.vhost, secure};
}

std::chrono::milliseconds HalfInterval(std::uint16_t interval_seconds) {
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::seconds{interval_seconds} / 2.0);
}

}  // namespace

AmqpConnectionHandler::AmqpConnectionHandler(
    clients::dns::Resolver& resolver,
    const EndpointInfo& endpoint,
    const AuthSettings& auth_settings,
    std::size_t heartbeat_interval_seconds,
    bool secure,
    statistics::ConnectionStatistics& stats,
    engine::Deadline deadline
)
    : address_{ToAmqpAddress(endpoint, auth_settings, secure)},
      socket_{CreateSocketPtr(resolver, address_, auth_settings, deadline)},
      reader_{*this, *socket_},
      configured_heartbeat_seconds_{static_cast<
          std::uint16_t>(std::min<std::size_t>(heartbeat_interval_seconds, std::numeric_limits<std::uint16_t>::max()))},
      stats_{stats}
{}

AmqpConnectionHandler::~AmqpConnectionHandler() {
    heartbeat_task_.Stop();
    reader_.Stop();
}

void AmqpConnectionHandler::onProperties(AMQP::Connection*, const AMQP::Table&, AMQP::Table& client) {
    client["product"] = "uServer AMQP library";
    client["copyright"] = "Copyright 2022-2022 Yandex NV";
    client["information"] = "https://userver.tech/dd/de2/rabbitmq_driver.html";
}

std::uint16_t AmqpConnectionHandler::onNegotiate(AMQP::Connection*, std::uint16_t interval) {
    const auto negotiated = std::min<std::uint16_t>(interval, configured_heartbeat_seconds_);
    negotiated_heartbeat_seconds_.store(negotiated, std::memory_order_relaxed);
    LOG_INFO() << "RabbitMQ heartbeat negotiated at " << negotiated << "s";
    return negotiated;
}

void AmqpConnectionHandler::onData(AMQP::Connection* connection, const char* buffer, std::size_t size) {
    if (IsBroken()) {
        // No further actions can be done
        return;
    }

    try {
        const auto sent = socket_->WriteAll(buffer, size, operation_deadline_);
        if (sent != size) {
            throw std::runtime_error{"Connection reset by peer"};
        }

        AccountWrite(size);
    } catch (const std::exception& ex) {
        LOG_ERROR() << "Failed to send data to socket: " << ex;
        Invalidate();

        // We do fail all the outstanding operations with this,
        // but it should be ok since we limit them by AmqpConnection::GetAwaiter().
        // There's no easy way to fail only the current operation,
        // so it's a compromise between allowing more throughput
        // (connection is returned to pool without waiting for response)
        // and error-rate. This behavior is documented in client_settings
        connection->fail("Underlying connection broke.");
    }
}

void AmqpConnectionHandler::onError(AMQP::Connection*, const char* message) {
    Invalidate();
    if (is_ready_) {
        return;
    }

    is_ready_.store(true);
    error_.emplace(message);
    connection_ready_event_.Send();
}

void AmqpConnectionHandler::onClosed(AMQP::Connection*) { Invalidate(); }

void AmqpConnectionHandler::onReady(AMQP::Connection*) {
    if (is_ready_) {
        // this shouldn't actually happen
        return;
    }

    is_ready_.store(true);
    connection_ready_event_.Send();
}

void AmqpConnectionHandler::OnConnectionCreated(AmqpConnection* connection, engine::Deadline deadline) {
    UINVARIANT(connection_ == nullptr, "Unexpected repeated OnConnectionCreated call");
    connection_ = connection;
    reader_.Start(connection);

    if (!connection_ready_event_.WaitForEventUntil(deadline)) {
        reader_.Stop();
        connection_ = nullptr;
        throw ConnectionSetupTimeout{"Failed to setup a connection within specified deadline"};
    }

    if (error_.has_value()) {
        reader_.Stop();
        connection_ = nullptr;
        throw ConnectionSetupError{"Failed to setup a connection: " + *error_};
    }

    const auto heartbeat_seconds = negotiated_heartbeat_seconds_.load(std::memory_order_relaxed);
    if (heartbeat_seconds > 0) {
        heartbeat_task_
            .Start("amqp_heartbeat", {HalfInterval(heartbeat_seconds), utils::PeriodicTask::Flags::kNow}, [this] {
                SendHeartbeat();
            });
    }
}

void AmqpConnectionHandler::OnConnectionDestruction() {
    heartbeat_task_.Stop();
    connection_ = nullptr;
    reader_.Stop();
}

void AmqpConnectionHandler::Invalidate() { broken_ = true; }

bool AmqpConnectionHandler::IsBroken() const { return broken_.load(); }

void AmqpConnectionHandler::AccountRead(std::size_t size) { stats_.AccountRead(size); }

void AmqpConnectionHandler::AccountWrite(std::size_t size) { stats_.AccountWrite(size); }

void AmqpConnectionHandler::SetOperationDeadline(engine::Deadline deadline) { operation_deadline_ = deadline; }

statistics::ConnectionStatistics& AmqpConnectionHandler::GetStatistics() { return stats_; }

const AMQP::Address& AmqpConnectionHandler::GetAddress() const { return address_; }

void AmqpConnectionHandler::SendHeartbeat() {
    LOG_DEBUG() << "Sending AMQP heartbeat";

    if (IsBroken() || connection_ == nullptr) {
        LOG_DEBUG("Failed to send AMQP heartbeat. Connection is broken");
        return;
    }

    try {
        const auto deadline = engine::Deadline::FromDuration(HalfInterval(configured_heartbeat_seconds_));
        auto lock = AmqpConnectionLocker{*connection_}.Lock(deadline);
        connection_->SetOperationDeadline(deadline);
        connection_->GetNative().heartbeat();
    } catch (const std::exception& ex) {
        LOG_WARNING() << "Failed to send AMQP heartbeat: " << ex.what();
        Invalidate();
        if (connection_ != nullptr) {
            auto lock = AmqpConnectionLocker{*connection_}.Lock({});
            connection_->GetNative().fail("Underlying connection broke.");
        }
    }
}

}  // namespace urabbitmq::impl

USERVER_NAMESPACE_END
