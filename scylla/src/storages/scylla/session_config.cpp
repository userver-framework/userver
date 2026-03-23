#include <userver/storages/scylla/session_config.hpp>

#include <gmock/internal/gmock-internal-utils.h>
#include <netinet/in.h>

#include <boost/iostreams/filter/gzip.hpp>

#include "userver/storages/scylla/session.hpp"

#include <userver/utils/text.hpp>
#include <userver/utils/trivial_map.hpp>

#include <userver/storages/scylla/exception.hpp>

#include "userver/server/request/task_inherited_data.hpp"

USERVER_NAMESPACE_BEGIN

namespace storages::scylla {

namespace {
bool IsValidAppName(const std::string& app_name) {
    bool is_utf8 = utils::text::IsUtf8(app_name);

    return is_utf8 && utils::text::IsCString(app_name);
}

void IsValidDuration(const std::chrono::milliseconds& timeout, const char* field_name, const std::string& session_id) {
    auto count_ms = timeout.count();

    bool is_valid = count_ms >= 0 && count_ms <= std::numeric_limits<int32_t>::max();

    if (!is_valid) {
        throw InvalidConfigException("Invalid ")
            << field_name << " in " << session_id << " pool config: " << count_ms << "ms";
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

std::optional<SerialConsistency> SerialConsistencyFromRaw(int16_t value) {
    switch (value) {
        case 0x0008:
            return SerialConsistency::kSerial;
        case 0x0009:
            return SerialConsistency::kLocalSerial;
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

void SessionSettings::Validate(const std::string& session_id) const {
    bool unable_to_connect = max_size == 0 || establishing_limit == 0;
    bool idle_exceeds_max = idle_limit > max_size;
    bool initial_exceeds_idle = initial_size > idle_limit;

    if (unable_to_connect) {
        throw InvalidConfigException("invalid max_size or establishing_limit at ") << session_id << " session_config";
    }

    if (idle_exceeds_max) {
        throw InvalidConfigException("idle_limit exceeds max_size at ") << session_id << " session_config";
    }

    if (initial_exceeds_idle) {
        throw InvalidConfigException("initial_size exceeds idle_limit at ") << session_id << " session_config";
    }
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

SessionConfig Parse(const yaml_config::YamlConfig& config, formats::parse::To<SessionConfig>) {
    SessionConfig result{};
    result.conn_timeout = config["conn_timeout"].As<std::chrono::milliseconds>(result.conn_timeout);
    result.request_timeout = config["request_timeout"].As<std::chrono::milliseconds>(result.request_timeout);
    result.consistency = config["consistency"].As<Consistency>(result.consistency);
    result.serial_consistency = config["serial_consistency"].As<SerialConsistency>(result.serial_consistency);
    result.pool_size = config["pool_size"].As<std::size_t>(result.pool_size);
    result.shard_awareness = config["shard_awareness"].As<bool>(result.shard_awareness);
    result.retry_policy = config["retry_policy"].As<SessionConfig::RetryPolicyType>(result.retry_policy);
    result.app_name = config["app_name"].As<std::string>(result.app_name);
    result.default_keyspace = config["default_keyspace"].As<std::string>();
    return result;
}

void SessionConfig::Validate(const std::string& session_id) const {
    IsValidDuration(conn_timeout, "connection timeout", session_id);
    IsValidDuration(request_timeout, "request timeout", session_id);

    dynamic_settings.Validate(session_id);

    if (!IsValidAppName(app_name)) {
        throw InvalidConfigException("Invalid app name in ") << session_id << " session config";
    }
}

}  // namespace storages::scylla

USERVER_NAMESPACE_END