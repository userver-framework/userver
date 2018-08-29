#include "redis_secdist.hpp"

#include <json/value.h>

#include <logging/log.hpp>
#include <storages/secdist/exceptions.hpp>
#include <storages/secdist/helpers.hpp>

namespace storages {
namespace secdist {

const ::secdist::RedisSettings& RedisMapSettings::GetSettings(
    const std::string& client_name) const {
  auto it = redis_settings_.find(client_name);
  if (it == redis_settings_.end()) {
    throw UnknownRedisClientName("client_name " + client_name +
                                 " not found in secdist config");
  }
  return it->second;
}

RedisMapSettings::RedisMapSettings(const Json::Value& doc) {
  static const int kDefaultSentinelPort = 26379;

  const auto& redis_settings = doc["redis_settings"];
  CheckIsObject(redis_settings, "redis_settings");

  for (auto it = redis_settings.begin(); it != redis_settings.end(); ++it) {
    auto client_name = it.key().asString();
    const auto& client_settings = *it;
    CheckIsObject(client_settings, "client_settings");

    ::secdist::RedisSettings settings;
    settings.password = GetString(client_settings, "password");

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
      ::secdist::RedisSettings::HostPort host_port;
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

    const auto& command_control = client_settings["command_control"];
    if (command_control.isObject()) {
      settings.command_control.timeout_single = std::chrono::milliseconds(
          GetInt(command_control, "timeout_single_ms",
                 settings.command_control.timeout_single.count()));
      if (settings.command_control.timeout_single.count() < 0) {
        throw InvalidSecdistJson("timeout_single_ms < 0");
      }
      settings.command_control.timeout_all = std::chrono::milliseconds(
          GetInt(command_control, "timeout_all_ms",
                 settings.command_control.timeout_all.count()));
      if (settings.command_control.timeout_all.count() < 0) {
        throw InvalidSecdistJson("timeout_all < 0");
      }
      settings.command_control.max_retries = GetInt(
          command_control, "max_retries", settings.command_control.max_retries);
      if (settings.command_control.max_retries < 0) {
        throw InvalidSecdistJson("max_retries < 0");
      }
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

}  // namespace secdist
}  // namespace storages
