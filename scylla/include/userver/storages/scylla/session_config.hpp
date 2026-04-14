#pragma once

#include <chrono>
#include <optional>
#include <string>
#include <vector>

#include <userver/yaml_config/yaml_config.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::scylla {
using RequestTimeout = std::chrono::milliseconds;

enum class Consistency : uint16_t {
    kAny = 0x0000,
    kOne = 0x0001,
    kTwo = 0x0002,
    kThree = 0x0003,
    kQuorum = 0x0004,
    kAll = 0x0005,
    kLocalQuorum = 0x0006,
    kEachQuorum = 0x0007,
    kLocalOne = 0x000A,
};

enum class SerialConsistency : uint16_t { kSerial = 0x0008, kLocalSerial = 0x0009 };

struct SessionSettings final {
    /// Number of I/O threads in the CassSession (handles async I/O)
    static constexpr std::size_t kDefaultNumThreadsIo = 1;
    /// Per-host core (minimum) connections kept alive
    static constexpr std::size_t kDefaultCoreConnectionsPerHost = 1;

    size_t num_threads_io = kDefaultNumThreadsIo;
    size_t core_connections_per_host = kDefaultCoreConnectionsPerHost;

    void Validate(const std::string& session_id) const;
};

struct SessionConfig final {
    // these are timeouts!!!
    static constexpr auto kDefaultConnTimeout = std::chrono::seconds{2};
    static constexpr auto kDefaultRequestTimeout = std::chrono::seconds{10};

    static constexpr std::size_t kDefaultPoolSize = 16;
    static constexpr char kDefaultAppName[] = "userver";

    std::chrono::milliseconds conn_timeout = kDefaultConnTimeout;
    std::chrono::milliseconds request_timeout = kDefaultRequestTimeout;

    Consistency consistency = Consistency::kLocalQuorum;
    SerialConsistency serial_consistency = SerialConsistency::kLocalSerial;

    std::size_t pool_size = kDefaultPoolSize;

    enum class LoadBalancingPolicy {
        kRoundRobin,
        kDcAware,
    };
    LoadBalancingPolicy load_balancing_policy = LoadBalancingPolicy::kDcAware;
    std::string preferred_datacenter;

    /// Token-aware routing: sends queries directly to the replica that owns
    /// the partition key. In ScyllaDB, this also enables shard-aware routing
    // (sends to the exact CPU shard within the node).
    /// Sits on top of the base load balancing policy.
    bool token_aware_routing = true;

    SessionSettings dynamic_settings;

    void Validate(const std::string& session_id) const;

    enum class RetryPolicyType {
        kDefault,
        kFallthrough,
    };
    RetryPolicyType retry_policy = RetryPolicyType::kDefault;

    struct SpeculativeExecution {
        bool enabled = false;
        std::size_t max_attempts = 2;
        std::chrono::milliseconds delay{100};
    };
    SpeculativeExecution speculative_execution;

    std::string default_keyspace;

    std::string app_name = kDefaultAppName;

    struct SslSettings {
        bool enabled = false;

        enum class VerifyMode {
            kNone,
            kPeerCert,
            kPeerIdentity,
            kPeerIdentityDns,
        };
        VerifyMode verify = VerifyMode::kPeerCert;
    };
    SslSettings ssl;
};

SessionConfig Parse(const yaml_config::YamlConfig& config, formats::parse::To<SessionConfig>);

/// TLS certificate material from secdist. Kept alongside SessionConfig
/// because the two are consumed together when building a CassCluster.
struct SslSecrets {
    std::vector<std::string> trusted_certs;
    std::optional<std::string> client_cert;
    std::optional<std::string> client_key;
    std::string client_key_password;
};

}  // namespace storages::scylla

USERVER_NAMESPACE_END