#include "session_impl.hpp"

#include <cctype>
#include <chrono>
#include <memory>

#include <cassandra.h>

#include <userver/crypto/openssl.hpp>
#include <userver/dynamic_config/source.hpp>
#include <userver/logging/log.hpp>
#include <userver/storages/scylla/exception.hpp>

#include <storages/scylla/driver/cass_wrappers.hpp>
#include <storages/scylla/driver/scylla_error.hpp>
#include <storages/scylla/scylla_secdist.hpp>
#include <storages/scylla/stats.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::scylla::impl::driver {

namespace {

constexpr const char* kPingQuery = "SELECT release_version FROM system.local";

// Maximum length of an unquoted CQL identifier (Cassandra/ScyllaDB limit).
constexpr std::size_t kMaxCqlIdentifierLength = 48;

std::chrono::milliseconds ElapsedSince(std::chrono::steady_clock::time_point start) noexcept {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start);
}

// Validate a string as a safe-to-interpolate unquoted CQL identifier.
//
// CQL DDL statements like DROP KEYSPACE cannot use bound parameters, so the
// keyspace name must be inlined into the query string. To prevent CQL
// injection we accept only the conservative subset: alphanumerics and
// underscores, starting with a letter, up to 48 characters.
bool IsValidCqlIdentifier(std::string_view name) noexcept {
    if (name.empty() || name.size() > kMaxCqlIdentifierLength) return false;
    if (std::isalpha(static_cast<unsigned char>(name.front())) == 0) return false;
    for (const char c : name) {
        const auto uc = static_cast<unsigned char>(c);
        if (std::isalnum(uc) == 0 && c != '_') return false;
    }
    return true;
}

void ApplyLoadBalancingPolicy(CassCluster* cluster, const SessionConfig& config) {
    using LBP = SessionConfig::LoadBalancingPolicy;

    // Step 1: Set the base routing policy
    switch (config.load_balancing_policy) {
        case LBP::kRoundRobin:
            cass_cluster_set_load_balance_round_robin(cluster);
            break;
        case LBP::kDcAware:
            cass_cluster_set_load_balance_dc_aware(
                cluster,
                config.preferred_datacenter.c_str(),
                0,         // used_hosts_per_remote_dc
                cass_false // don't allow remote DCs for local consistency
            );
            break;
    }

    // Step 2: Token-aware is an optimization layer on top of any base policy.
    // In ScyllaDB, this also enables shard-aware routing (requests go to the
    // exact CPU shard that owns the partition, not just the node).
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
    const std::optional<secdist::SslSecrets>& ssl_secrets
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

}  // namespace

DriverSessionImpl::DriverSessionImpl(
    std::string id,
    const std::string& hosts,
    const SessionConfig& session_config,
    dynamic_config::Source config_source,
    clients::dns::Resolver* /*dns_resolver*/,
    std::optional<secdist::SslSecrets> ssl_secrets
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
    LOG_DEBUG() << "Creating scylla connection";

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

    if (!session_config_.app_name.empty()) {
        cass_cluster_set_application_name(cluster.get(),
            session_config_.app_name.c_str());
    }

    ApplySsl(cluster.get(), session_config_.ssl, ssl_secrets_);

    CassSessionPtr session(cass_session_new());

    CassFuturePtr connect_future(
        cass_session_connect(session.get(), cluster.get()));
    cass_future_wait(connect_future.get());
    CheckFuture(connect_future.get(), "connect");

    auto& session_stats = *GetStatistics().session;
    {
        const auto start = std::chrono::steady_clock::now();
        try {
            CassStatementPtr ping_stmt(cass_statement_new(kPingQuery, 0));
            CassFuturePtr ping_future(
                cass_session_execute(session.get(), ping_stmt.get()));
            cass_future_wait(ping_future.get());
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
    auto& session_stats = *GetStatistics().session;
    const auto start = std::chrono::steady_clock::now();
    try {
        CassStatementPtr ping_stmt(cass_statement_new(kPingQuery, 0));
        CassFuturePtr ping_future(
            cass_session_execute(connection_->GetSession(), ping_stmt.get()));
        cass_future_wait(ping_future.get());
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
    if (!IsValidCqlIdentifier(default_keyspace_)) {
        throw InvalidConfigException(
            "Refusing to drop keyspace '" + default_keyspace_ +
            "': name is not a valid unquoted CQL identifier "
            "(letters/digits/underscore, starting with a letter, max 48 chars)");
    }

    LOG_WARNING() << "Dropping ScyllaDB keyspace '" << default_keyspace_
                  << "' on session '" << Id() << "'";

    // Safe to interpolate: default_keyspace_ has been validated above as a
    // restricted CQL identifier, so it cannot contain quotes, semicolons,
    // whitespace, or comment markers.
    const std::string query = "DROP KEYSPACE IF EXISTS " + default_keyspace_;

    auto& session_stats = *GetStatistics().session;
    const auto start = std::chrono::steady_clock::now();
    try {
        CassStatementPtr stmt(cass_statement_new(query.c_str(), 0));
        CassFuturePtr future(
            cass_session_execute(connection_->GetSession(), stmt.get()));
        cass_future_wait(future.get());
        CheckFuture(future.get(), "drop keyspace");

        session_stats.queries->Account(stats::ErrorType::kSuccess, ElapsedSince(start));
    } catch (const std::exception& ex) {
        session_stats.queries->Account(stats::ClassifyException(ex), ElapsedSince(start));
        throw;
    }
}

const std::string& DriverSessionImpl::DefaultDatabaseName() const { return default_keyspace_; }

}  // namespace storages::scylla::impl::driver

USERVER_NAMESPACE_END
