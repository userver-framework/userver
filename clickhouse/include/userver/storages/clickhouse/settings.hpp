#pragma once

#include <chrono>
#include <string>
#include <unordered_map>

#include <userver/components/component_fwd.hpp>
#include <userver/formats/json/value.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::clickhouse {

struct AuthSettings final {
  std::string user;
  std::string password;
  std::string database;

  // for testing purposes only, dont use directly
  AuthSettings();

  AuthSettings(const formats::json::Value&);

  void SetDatabase(const std::string& dbname);
};

struct EndpointSettings final {
  std::string host;
  uint32_t port;
};

struct PoolSettings final {
  size_t initial_pool_size;
  size_t max_pool_size;
  std::chrono::milliseconds queue_timeout;
  bool use_secure_connection;

  EndpointSettings endpoint_settings;
  AuthSettings auth_settings;

  PoolSettings(const components::ComponentConfig&, const EndpointSettings&,
               const AuthSettings&);
};

struct ClickhouseSettings final {
  std::vector<EndpointSettings> endpoints;

  AuthSettings auth_settings;

  // for testing purposes only, dont use directly
  ClickhouseSettings();

  ClickhouseSettings(const formats::json::Value&);
};

class ClickhouseSettingsMulti final {
 public:
  ClickhouseSettingsMulti(const formats::json::Value&);

  const ClickhouseSettings& Get(const std::string& dbname) const;

 private:
  std::unordered_map<std::string, ClickhouseSettings> databases_;
};

std::string GetDbName(const components::ComponentConfig&);

}  // namespace storages::clickhouse

USERVER_NAMESPACE_END
