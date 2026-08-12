#include <userver/storages/scylla/session_config.hpp>

#include <gmock/internal/gmock-internal-utils.h>
#include <netinet/in.h>

#include <userver/utils/text.hpp>
#include <userver/utils/trivial_map.hpp>

#include <userver/storages/scylla/exception.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::scylla {

namespace {
bool IsValidAppName(std::string_view app_name) {
    bool is_utf8 = utils::text::IsUtf8(app_name);

    return is_utf8 && utils::text::IsCString(app_name);
}

void IsValidDuration(const std::chrono::milliseconds& timeout, const char* field_name, std::string_view session_id) {
    auto count_ms = timeout.count();

    bool is_valid = count_ms >= 0 && count_ms <= std::numeric_limits<int32_t>::max();

    if (!is_valid) {
        throw InvalidConfigException("Invalid "
        ) << field_name
          << " in " << session_id << " pool config: " << count_ms << "ms";
    }
}

constexpr Consistency kDefaultConsistency = Consistency::kLocalQuorum;
constexpr SerialConsistency kDefaultSerialConsistency = SerialConsistency::kLocalSerial;

std::optional<Consistency> ConsistencyFromRaw(uint16_t value) {
    switch (value) {
        case 0x0000:
            return Consistency::kAny;
        case 0x0001:
            return Consistency::kOne;
        case 0x0002:
            return Consistency::kTwo;
        case 0x0003:
            return Consistency::kThree;
        case 0x0004:
            return Consistency::kQuorum;
        case 0x0005:
            return Consistency::kAll;
        case 0x0006:
            return Consistency::kLocalQuorum;
        case 0x0007:
            return Consistency::kEachQuorum;
        case 0x000A:
            return Consistency::kLocalOne;
        default:
            return std::nullopt;
    }
}

}  // namespace

constexpr utils::TrivialBiMap kSerialConsistencyMapping([](auto selector) {
    return selector().Case(SerialConsistency::kSerial, "serial").Case(SerialConsistency::kLocalSerial, "local_serial");
});

constexpr utils::TrivialBiMap kConsistencyMapping([](auto selector) {
    return selector()
        .Case(Consistency::kAny, "any")
        .Case(Consistency::kOne, "one")
        .Case(Consistency::kTwo, "two")
        .Case(Consistency::kThree, "three")
        .Case(Consistency::kQuorum, "quorum")
        .Case(Consistency::kAll, "all")
        .Case(Consistency::kLocalQuorum, "local_quorum")
        .Case(Consistency::kEachQuorum, "each_quorum")
        .Case(Consistency::kLocalOne, "local_one");
});

constexpr utils::TrivialBiMap kRetryPolicyMapping([](auto selector) {
    return selector()
        .Case(SessionConfig::RetryPolicyType::kDefault, "default")
        .Case(SessionConfig::RetryPolicyType::kFallthrough, "fallthrough");
});

constexpr utils::TrivialBiMap kLoadBalancingPolicyMapping([](auto selector) {
    return selector()
        .Case(SessionConfig::LoadBalancingPolicy::kRoundRobin, "round_robin")
        .Case(SessionConfig::LoadBalancingPolicy::kDcAware, "dc_aware");
});

constexpr utils::TrivialBiMap kSslVerifyModeMapping([](auto selector) {
    return selector()
        .Case(SessionConfig::SslSettings::VerifyMode::kNone, "none")
        .Case(SessionConfig::SslSettings::VerifyMode::kPeerCert, "peer_cert")
        .Case(SessionConfig::SslSettings::VerifyMode::kPeerIdentity, "peer_identity")
        .Case(SessionConfig::SslSettings::VerifyMode::kPeerIdentityDns, "peer_identity_dns");
});

void SessionPoolSettings::Validate(std::string_view session_id) const {
    if (num_threads_io == 0) {
        throw InvalidConfigException("num_threads_io must be > 0 at ") << session_id;
    }
    if (core_connections_per_host == 0) {
        throw InvalidConfigException("core_connections_per_host must be > 0 at ") << session_id;
    }
}

SessionPoolSettings Parse(const yaml_config::YamlConfig& config, formats::parse::To<SessionPoolSettings>) {
    SessionPoolSettings result{};

    result.num_threads_io = config["num_threads_io"].As<std::size_t>(result.num_threads_io);
    result.core_connections_per_host =
        config["core_connections_per_host"].As<std::size_t>(result.core_connections_per_host);
    result.core_connections_per_shard =
        config["core_connections_per_shard"].As<std::size_t>(result.core_connections_per_shard);
    return result;
}

static auto Parse(const yaml_config::YamlConfig& config, formats::parse::To<Consistency>) {
    return utils::ParseFromValueString<InvalidConfigException>(config, kConsistencyMapping);
}

static auto Parse(const yaml_config::YamlConfig& config, formats::parse::To<SerialConsistency>) {
    return utils::ParseFromValueString<InvalidConfigException>(config, kSerialConsistencyMapping);
}

static auto Parse(const yaml_config::YamlConfig& config, formats::parse::To<SessionConfig::RetryPolicyType>) {
    return utils::ParseFromValueString<InvalidConfigException>(config, kRetryPolicyMapping);
}

static auto Parse(const yaml_config::YamlConfig& config, formats::parse::To<SessionConfig::LoadBalancingPolicy>) {
    return utils::ParseFromValueString<InvalidConfigException>(config, kLoadBalancingPolicyMapping);
}

static auto Parse(const yaml_config::YamlConfig& config, formats::parse::To<SessionConfig::SslSettings::VerifyMode>) {
    return utils::ParseFromValueString<InvalidConfigException>(config, kSslVerifyModeMapping);
}

SessionConfig Parse(const yaml_config::YamlConfig& config, formats::parse::To<SessionConfig>) {
    SessionConfig result{};
    result.conn_timeout = config["conn_timeout"].As<std::chrono::milliseconds>(result.conn_timeout);
    result.request_timeout = config["request_timeout"].As<std::chrono::milliseconds>(result.request_timeout);
    result.consistency = config["consistency"].As<Consistency>(result.consistency);
    result.serial_consistency = config["serial_consistency"].As<SerialConsistency>(result.serial_consistency);
    result.pool_settings = config["pool"].As<SessionPoolSettings>(result.pool_settings);
    result.token_aware_routing = config["token_aware_routing"].As<bool>(result.token_aware_routing);
    result.retry_policy = config["retry_policy"].As<SessionConfig::RetryPolicyType>(result.retry_policy);
    result.load_balancing_policy =
        config["load_balancing_policy"].As<SessionConfig::LoadBalancingPolicy>(result.load_balancing_policy);
    result.preferred_datacenter = config["preferred_datacenter"].As<std::string>(result.preferred_datacenter);
    result.app_name = config["app_name"].As<std::string>(result.app_name);
    result.default_keyspace = config["default_keyspace"].As<std::string>();

    if (config.HasMember("speculative_execution")) {
        const auto& spec = config["speculative_execution"];
        result.speculative_execution.enabled = spec["enabled"].As<bool>(result.speculative_execution.enabled);
        result.speculative_execution
            .max_attempts = spec["max_attempts"].As<std::size_t>(result.speculative_execution.max_attempts);
        result.speculative_execution
            .delay = spec["delay"].As<std::chrono::milliseconds>(result.speculative_execution.delay);
    }

    if (config.HasMember("ssl")) {
        const auto& ssl = config["ssl"];
        result.ssl.enabled = ssl["enabled"].As<bool>(result.ssl.enabled);
        result.ssl.verify = ssl["verify"].As<SessionConfig::SslSettings::VerifyMode>(result.ssl.verify);
    }

    return result;
}

void SessionConfig::Validate(std::string_view session_id) const {
    IsValidDuration(conn_timeout, "connection timeout", session_id);
    IsValidDuration(request_timeout, "request timeout", session_id);

    pool_settings.Validate(session_id);

    if (!IsValidAppName(app_name)) {
        throw InvalidConfigException("Invalid app name in ") << session_id << " session config";
    }

    if (load_balancing_policy == LoadBalancingPolicy::kDcAware && preferred_datacenter.empty()) {
        throw InvalidConfigException("preferred_datacenter is required for dc_aware load balancing in ") << session_id;
    }
}

}  // namespace storages::scylla

USERVER_NAMESPACE_END
