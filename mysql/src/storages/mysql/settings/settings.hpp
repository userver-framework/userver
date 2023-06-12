#pragma once

#include <unordered_map>
#include <vector>

#include <userver/components/component_fwd.hpp>
#include <userver/formats/json_fwd.hpp>
#include <userver/utils/strong_typedef.hpp>
#include <userver/yaml_config/fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::settings {

struct AuthSettings final {
  std::string database;
  std::string user;
  utils::NonLoggable<struct PasswordTag, std::string> password;

  ~AuthSettings();
};

AuthSettings Parse(const formats::json::Value& doc,
                   formats::parse::To<AuthSettings>);

enum class IpMode { kIpV4, kIpV6, kAny };

struct ConnectionSettings final {
  std::size_t statements_cache_size;
  // TODO : implement ssl somehow
  bool use_secure_connection;
  // TODO : implement compression somehow
  bool use_compression;

  IpMode ip_mode;
};

ConnectionSettings Parse(const yaml_config::YamlConfig& doc,
                         formats::parse::To<ConnectionSettings>);

struct EndpointInfo final {
  std::string host;
  std::uint32_t port;
};

struct PoolSettings final {
  std::size_t initial_pool_size{5};
  std::size_t max_pool_size{10};

  EndpointInfo endpoint_info;
  AuthSettings auth_settings;
  ConnectionSettings connection_settings;

  static PoolSettings Create(const components::ComponentConfig& config,
                             const EndpointInfo& endpoint_info,
                             const AuthSettings& auth);
};

struct MysqlSettings final {
  std::vector<EndpointInfo> endpoints;

  AuthSettings auth;
};

MysqlSettings Parse(const formats::json::Value& value,
                    formats::parse::To<MysqlSettings>);

class MysqlSettingsMulti final {
 public:
  explicit MysqlSettingsMulti(const formats::json::Value& secdist);

  const MysqlSettings& Get(const std::string& dbname) const;

 private:
  std::unordered_map<std::string, MysqlSettings> databases_;
};

std::string GetSecdistAlias(const components::ComponentConfig& config);

}  // namespace storages::mysql::settings

USERVER_NAMESPACE_END
