#pragma once

#include <chrono>
#include <optional>
#include <string>

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
    static constexpr std::size_t kDefaultInitialSize = 16;
    static constexpr std::size_t kDefaultMaxSize = 128;
    static constexpr std::size_t kDefaultIdleLimit = 64;
    static constexpr std::size_t kDefaultConnectingLimit = 8;

    size_t initial_size = kDefaultInitialSize;
    size_t max_size = kDefaultMaxSize;
    size_t idle_limit = kDefaultIdleLimit;
    size_t establishing_limit = kDefaultConnectingLimit;

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
        kTokenAwareRoundRobin,
        kDcAwareRoundRobin,
    };
    LoadBalancingPolicy load_balancing_policy = LoadBalancingPolicy::kTokenAwareRoundRobin;
    std::string preferred_datacenter;

    SessionSettings dynamic_settings;

    void Validate(const std::string& session_id) const;

    bool shard_awareness = true;

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
};

SessionConfig Parse(const yaml_config::YamlConfig& config, formats::parse::To<SessionConfig>);
}  // namespace storages::scylla

USERVER_NAMESPACE_END