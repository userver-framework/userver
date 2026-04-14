#include "session_impl.hpp"

#include <chrono>
#include <memory>
#include <string_view>

#include <cassandra.h>

#include <userver/crypto/openssl.hpp>
#include <userver/dynamic_config/source.hpp>
#include <userver/logging/log.hpp>
#include <userver/storages/scylla/exception.hpp>
#include <userver/tracing/span.hpp>
#include <userver/tracing/tags.hpp>

#include <storages/scylla/driver/async_future.hpp>
#include <storages/scylla/driver/cass_wrappers.hpp>
#include <storages/scylla/driver/cql_builder.hpp>
#include <storages/scylla/driver/query_helpers.hpp>
#include <storages/scylla/driver/request_context.hpp>
#include <storages/scylla/driver/scylla_error.hpp>
#include <storages/scylla/scylla_secdist.hpp>
#include <storages/scylla/stats.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::scylla::impl::driver {

namespace {

constexpr const char* kPingQuery = "SELECT release_version FROM system.local";

std::chrono::milliseconds ElapsedSince(std::chrono::steady_clock::time_point start) noexcept {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start);
}

void ApplyLoadBalancingPolicy(CassCluster* cluster, const SessionConfig& config) {
    using LBP = SessionConfig::LoadBalancingPolicy;

    switch (config.load_balancing_policy) {
        case LBP::kRoundRobin:
            cass_cluster_set_load_balance_round_robin(cluster);
            break;
        case LBP::kDcAware:
            cass_cluster_set_load_balance_dc_aware(
                cluster,
                config.preferred_datacenter.c_str(),
                0,
                cass_false
            );
            break;
    }

    cass_cluster_set_token_aware_routing(cluster,
        config.token_aware_routing ? cass_true : cass_false);
}

void ApplyRetryPolicy(CassCluster* cluster, SessionConfig::RetryPolicyType type) {
    CassRetryPolicyPtr policy;
    switch (type) {
        case SessionConfig::RetryPolicyType::kDefault:
            policy.reset(cass_retry_policy_default_new());
            break;
        case SessionConfig::RetryPolicyType::kFallthrough:
            policy.reset(cass_retry_policy_fallthrough_new());
            break;
    }
    cass_cluster_set_retry_policy(cluster, policy.get());
}

int ToVerifyFlags(SessionConfig::SslSettings::VerifyMode mode) {
    using VM = SessionConfig::SslSettings::VerifyMode;
    switch (mode) {
        case VM::kNone:
            return CASS_SSL_VERIFY_NONE;
        case VM::kPeerCert:
            return CASS_SSL_VERIFY_PEER_CERT;
        case VM::kPeerIdentity:
            return CASS_SSL_VERIFY_PEER_CERT | CASS_SSL_VERIFY_PEER_IDENTITY;
        case VM::kPeerIdentityDns:
            return CASS_SSL_VERIFY_PEER_CERT | CASS_SSL_VERIFY_PEER_IDENTITY_DNS;
    }
    return CASS_SSL_VERIFY_PEER_CERT;
}

void ApplySsl(
    CassCluster* cluster,
    const SessionConfig::SslSettings& ssl_config,
    const std::optional<SslSecrets>& ssl_secrets
) {
    if (!ssl_config.enabled) return;

    crypto::Openssl::Init();

    CassSslPtr ssl(cass_ssl_new());
    cass_ssl_set_verify_flags(ssl.get(), ToVerifyFlags(ssl_config.verify));

    if (ssl_secrets) {
        for (const auto& cert : ssl_secrets->trusted_certs) {
            CassError rc = cass_ssl_add_trusted_cert(ssl.get(), cert.c_str());
            if (rc != CASS_OK) {
                throw ScyllaException() << "Failed to add trusted cert: " << cass_error_desc(rc);
            }
        }
        if (ssl_secrets->client_cert) {
            CassError rc = cass_ssl_set_cert(ssl.get(), ssl_secrets->client_cert->c_str());
            if (rc != CASS_OK) {
                throw ScyllaException() << "Failed to set client cert: " << cass_error_desc(rc);
            }
        }
        if (ssl_secrets->client_key) {
            CassError rc = cass_ssl_set_private_key(
                ssl.get(),
                ssl_secrets->client_key->c_str(),
                ssl_secrets->client_key_password.c_str()
            );
            if (rc != CASS_OK) {
                throw ScyllaException() << "Failed to set private key: " << cass_error_desc(rc);
            }
        }
    }

    cass_cluster_set_ssl(cluster, ssl.get());
}

void ApplySpeculativeExecution(CassCluster* cluster, const SessionConfig::SpeculativeExecution& spec) {
    if (spec.enabled) {
        cass_cluster_set_constant_speculative_execution_policy(
            cluster,
            static_cast<cass_int64_t>(spec.delay.count()),
            static_cast<int>(spec.max_attempts));
    } else {
        cass_cluster_set_no_speculative_execution_policy(cluster);
    }
}

void CppDriverLogCallback(const CassLogMessage* message, void* ) noexcept {
    if (!message) return;
    const std::string_view text(message->message);
    switch (message->severity) {
        case CASS_LOG_CRITICAL:
        case CASS_LOG_ERROR:
            LOG_ERROR() << "[scylla cpp-driver] " << text;
            break;
        case CASS_LOG_WARN:
            LOG_WARNING() << "[scylla cpp-driver] " << text;
            break;
        case CASS_LOG_INFO:
            LOG_INFO() << "[scylla cpp-driver] " << text;
            break;
        case CASS_LOG_DEBUG:
        case CASS_LOG_TRACE:
            LOG_DEBUG() << "[scylla cpp-driver] " << text;
            break;
        default:
            break;
    }
}

void EnsureCppDriverLogBridge() {
    [[maybe_unused]] static const int kBridgeInstalled = [] {
        cass_log_set_level(CASS_LOG_WARN);
        cass_log_set_callback(&CppDriverLogCallback, nullptr);
        return 0;
    }();
}

void ApplyDefaultConsistency(CassCluster* cluster, const SessionConfig& config) {
    cass_cluster_set_consistency(
        cluster, static_cast<CassConsistency>(config.consistency));
    cass_cluster_set_serial_consistency(
        cluster, static_cast<CassConsistency>(config.serial_consistency));
}

}

DriverSessionImpl::DriverSessionImpl(
    std::string id,
    const std::string& hosts,
    const SessionConfig& session_config,
    dynamic_config::Source config_source,
    clients::dns::Resolver* ,
    std::optional<SslSecrets> ssl_secrets
)
    : SessionImpl(std::move(id), session_config, config_source),
      session_config_(session_config),
      ssl_secrets_(std::move(ssl_secrets)),
      default_keyspace_(session_config.default_keyspace) {
    SetConnectionString(hosts);
}

void DriverSessionImpl::SetConnectionString(const std::string& connection_string) {
    if (hosts_ == connection_string) {
        return;
    }

    auto& session_stats = *GetStatistics().session;
    const bool is_reconnect = !hosts_.empty();
    if (is_reconnect) {
        LOG_WARNING() << "New connection string for '" << Id()
                      << "' found in secdist, reconnecting to ScyllaDB";
    }

    hosts_ = connection_string;
    auto new_connection = Create();
    if (connection_) {
        ++session_stats.closed;
    }
    connection_ = std::move(new_connection);
    if (is_reconnect) {
        ++session_stats.reconnects;
    }
}

DriverSessionImpl::ConnPtr DriverSessionImpl::Create() {
    EnsureCppDriverLogBridge();

    tracing::Span span("scylla_connect");
    span.AddTag(tracing::kDatabaseType, std::string{kDatabaseScyllaType});
    if (!default_keyspace_.empty()) {
        span.AddTag(tracing::kDatabaseInstance, default_keyspace_);
    }

    LOG_DEBUG() << "Creating scylla connection";

    prepared_cache_.Clear();

    CassClusterPtr cluster(cass_cluster_new());
    cass_cluster_set_contact_points(cluster.get(), hosts_.c_str());

    cass_cluster_set_connect_timeout(cluster.get(),
        static_cast<unsigned>(session_config_.conn_timeout.count()));
    cass_cluster_set_request_timeout(cluster.get(),
        static_cast<unsigned>(session_config_.request_timeout.count()));

    const auto& settings = session_config_.dynamic_settings;
    cass_cluster_set_num_threads_io(cluster.get(), settings.num_threads_io);
    cass_cluster_set_core_connections_per_host(cluster.get(),
        settings.core_connections_per_host);

    ApplyLoadBalancingPolicy(cluster.get(), session_config_);
    ApplyRetryPolicy(cluster.get(), session_config_.retry_policy);
    ApplySpeculativeExecution(cluster.get(), session_config_.speculative_execution);
    ApplyDefaultConsistency(cluster.get(), session_config_);

    if (!session_config_.app_name.empty()) {
        cass_cluster_set_application_name(cluster.get(),
            session_config_.app_name.c_str());
    }

    ApplySsl(cluster.get(), session_config_.ssl, ssl_secrets_);

    CassSessionPtr session(cass_session_new());

    CassFuturePtr connect_future(
        cass_session_connect(session.get(), cluster.get()));
    AsyncWaitFuture(connect_future.get());
    CheckFuture(connect_future.get(), "connect");

    auto& session_stats = *GetStatistics().session;
    {
        const auto start = std::chrono::steady_clock::now();
        try {
            CassStatementPtr ping_stmt(cass_statement_new(kPingQuery, 0));
            ApplyConsistency(ping_stmt.get(), session_config_, std::nullopt);
            MarkIdempotent(ping_stmt.get());
            CassFuturePtr ping_future(
                cass_session_execute(session.get(), ping_stmt.get()));
            AsyncWaitFuture(ping_future.get());
            CheckFuture(ping_future.get(), "ping");
            session_stats.ping->Account(stats::ErrorType::kSuccess, ElapsedSince(start));
        } catch (const std::exception& ex) {
            session_stats.ping->Account(stats::ClassifyException(ex), ElapsedSince(start));
            throw;
        }
    }

    ++session_stats.created;
    return std::make_shared<Connection>(session.release(), cluster.release());
}

void DriverSessionImpl::Ping() {
    auto span = MakeDbSpan("scylla_ping", default_keyspace_);
    auto& session_stats = *GetStatistics().session;
    const auto start = std::chrono::steady_clock::now();
    try {
        CassStatementPtr ping_stmt(cass_statement_new(kPingQuery, 0));
        ApplyConsistency(ping_stmt.get(), session_config_, std::nullopt);
        ApplyDeadlineAndTimeout(ping_stmt.get(), session_config_.request_timeout);
        MarkIdempotent(ping_stmt.get());
        CassFuturePtr ping_future(
            cass_session_execute(connection_->GetSession(), ping_stmt.get()));
        AsyncWaitFuture(ping_future.get());
        CheckFuture(ping_future.get(), "ping");
        session_stats.ping->Account(stats::ErrorType::kSuccess, ElapsedSince(start));
    } catch (const std::exception& ex) {
        session_stats.ping->Account(stats::ClassifyException(ex), ElapsedSince(start));
        throw;
    }
}

void DriverSessionImpl::DropKeyspace() {
    if (default_keyspace_.empty()) {
        throw InvalidConfigException(
            "Cannot drop keyspace: no default keyspace configured for session '" + Id() + "'");
    }

    LOG_WARNING() << "Dropping ScyllaDB keyspace '" << default_keyspace_
                  << "' on session '" << Id() << "'";

    auto context = MakeSessionRequestContext(*this, "scylla_drop_keyspace", default_keyspace_);

    auto statement = cql::BuildDropKeyspaceStatement(context);

    ExecuteStatement(context, std::move(statement), true, "drop keyspace");
}

const std::string& DriverSessionImpl::DefaultDatabaseName() const { return default_keyspace_; }

}

USERVER_NAMESPACE_END
