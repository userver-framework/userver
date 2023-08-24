#include "redis_secdist.hpp"

#include <userver/logging/log.hpp>
#include <userver/storages/secdist/exceptions.hpp>
#include <userver/storages/secdist/helpers.hpp>
#include "userver/storages/redis/impl/base.hpp"

USERVER_NAMESPACE_BEGIN

namespace storages::secdist {

const USERVER_NAMESPACE::secdist::RedisSettings& RedisMapSettings::GetSettings(
    const std::string& client_name) const {
  auto it = redis_settings_.find(client_name);
  if (it == redis_settings_.end()) {
    throw UnknownRedisClientName("client_name " + client_name +
                                 " not found in secdist config");
  }
  return it->second;
}

RedisMapSettings::RedisMapSettings(const formats::json::Value& doc) {
  static const int kDefaultSentinelPort = 26379;

  const auto& redis_settings = doc["redis_settings"];
  CheckIsObject(redis_settings, "redis_settings");

  for (auto it = redis_settings.begin(); it != redis_settings.end(); ++it) {
    auto client_name = it.GetName();
    const auto& client_settings = *it;
    CheckIsObject(client_settings, "client_settings");

    USERVER_NAMESPACE::secdist::RedisSettings settings;
    settings.password = USERVER_NAMESPACE::redis::Password(
        GetString(client_settings, "password"));
    settings.secure_connection =
        GetValue<bool>(client_settings, "secure_connection", false)
            ? USERVER_NAMESPACE::redis::ConnectionSecurity::kTLS
            : USERVER_NAMESPACE::redis::ConnectionSecurity::kNone;

    const auto& shards = client_settings["shards"];
    CheckIsArray(shards, "shards");
    for (const auto& shard : shards) {
      CheckIsObject(shard, "shard");
      settings.shards.push_back(GetString(shard, "name"));
    }

    const auto& sentinels = client_settings["sentinels"];
    CheckIsArray(sentinels, "sentinels");
    for (const auto& sentinel : sentinels) {
      CheckIsObject(sentinel, "sentinels");
      USERVER_NAMESPACE::secdist::RedisSettings::HostPort host_port;
      host_port.host = GetString(sentinel, "host");
      if (host_port.host.empty()) {
        throw InvalidSecdistJson("Empty redis sentinel host");
      }
      int port = GetInt(sentinel, "port", kDefaultSentinelPort);
      if (port <= 0 || port >= 65536) {
        throw InvalidSecdistJson("Invalid redis sentinel port");
      }
      host_port.port = port;
      settings.sentinels.push_back(std::move(host_port));
    }

    LOG_DEBUG() << "Added client '" << client_name << '\'';
    auto insertion_result =
        redis_settings_.emplace(std::move(client_name), std::move(settings));
    if (!insertion_result.second) {
      throw InvalidSecdistJson("Duplicate redis client_name: '" +
                               insertion_result.first->first + '\'');
    }
  }
}

}  // namespace storages::secdist

USERVER_NAMESPACE_END
