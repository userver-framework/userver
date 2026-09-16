#pragma once

#include <chrono>
#include <string>
#include <unordered_map>
#include <vector>

#include <userver/components/component_fwd.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/yaml_config/fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::clickhouse::impl {

struct AuthSettings final {
    std::string user;
    std::string password;
    std::string database;
};

AuthSettings Parse(const formats::json::Value&, formats::parse::To<AuthSettings>);

struct EndpointSettings final {
    std::string host;
    uint32_t port;
};

struct ConnectionSettings final {
    enum class ConnectionMode { kNonSecure, kSecure };

    enum class CompressionMethod { kNone, kLZ4 };

    ConnectionMode connection_mode{ConnectionMode::kSecure};

    CompressionMethod compression_method{CompressionMethod::kNone};
};

ConnectionSettings Parse(const yaml_config::YamlConfig&, formats::parse::To<ConnectionSettings>);

struct PoolSettings final {
    size_t initial_pool_size;
    size_t max_pool_size;
    std::chrono::milliseconds queue_timeout;

    EndpointSettings endpoint_settings;
    AuthSettings auth_settings;
    ConnectionSettings connection_settings;

    PoolSettings(const yaml_config::YamlConfig&, const EndpointSettings&, const AuthSettings&);
};

struct ClickhouseSettings final {
    std::vector<EndpointSettings> endpoints;

    AuthSettings auth_settings;
};

ClickhouseSettings Parse(const formats::json::Value&, formats::parse::To<ClickhouseSettings>);

class ClickhouseSettingsMulti final {
public:
    ClickhouseSettingsMulti(const formats::json::Value&);

    const ClickhouseSettings& Get(const std::string& dbname) const;

private:
    std::unordered_map<std::string, ClickhouseSettings> databases_;
};

std::string GetSecdistAlias(const components::ComponentConfig&);

}  // namespace storages::clickhouse::impl

USERVER_NAMESPACE_END
