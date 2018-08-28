#include <storages/secdist/secdist.hpp>

#include <cerrno>
#include <fstream>
#include <sstream>

#include <json/reader.h>

#include <json_config/value.hpp>

namespace storages {
namespace secdist {

namespace {

void ThrowInvalidSecdistType(const std::string& name, const std::string& type) {
  std::ostringstream os;
  os << '\'' << name << "' is not " << type << " (or not found)";
  throw InvalidSecdistJson(os.str());
}

std::string GetString(const Json::Value& parent_val, const std::string& name) {
  const auto& val = parent_val[name];
  if (!val.isString()) {
    ThrowInvalidSecdistType(name, "a string");
  }
  return val.asString();
}

int GetInt(const Json::Value& parent_val, const std::string& name, int dflt) {
  const auto& val = parent_val[name];
  if (val.isNull()) {
    return dflt;
  }
  if (!val.isInt()) {
    ThrowInvalidSecdistType(name, "an int");
  }
  return val.asInt();
}

void CheckIsObject(const Json::Value& val, const std::string& name) {
  if (!val.isObject()) {
    ThrowInvalidSecdistType(name, "an object");
  }
}

void CheckIsArray(const Json::Value& val, const std::string& name) {
  if (!val.isArray()) {
    ThrowInvalidSecdistType(name, "an array");
  }
}

}  // namespace

SecdistConfig::SecdistConfig() = default;

SecdistConfig::SecdistConfig(const std::string& path) {
  std::ifstream json_stream(path);

  if (!json_stream) {
    std::error_code ec(errno, std::system_category());
    throw InvalidSecdistJson("Cannot load secdist config: " + ec.message());
  }

  Json::Reader reader;
  Json::Value doc;

  if (!reader.parse(json_stream, doc)) {
    throw InvalidSecdistJson("Cannot load secdist config: " +
                             reader.getFormattedErrorMessages());
  }

  LoadMongoSettings(doc);
  LoadRedisSettings(doc);
}

const std::string& SecdistConfig::GetMongoConnectionString(
    const std::string& dbalias) const {
  auto it = mongo_settings_.find(dbalias);
  if (it == mongo_settings_.end()) {
    throw UnknownMongoDbAlias("dbalias " + dbalias +
                              " not found in secdist config");
  }
  return it->second;
}

const ::secdist::RedisSettings& SecdistConfig::GetRedisSettings(
    const std::string& client_name) const {
  auto it = redis_settings_.find(client_name);
  if (it == redis_settings_.end()) {
    throw UnknownRedisClientName("client_name " + client_name +
                                 " not found in secdist config");
  }
  return it->second;
}

void SecdistConfig::LoadMongoSettings(const Json::Value& doc) {
  const auto& mongo_settings = doc["mongo_settings"];
  CheckIsObject(mongo_settings, "mongo_settings");

  for (auto it = mongo_settings.begin(); it != mongo_settings.end(); ++it) {
    auto dbalias = it.key().asString();
    const auto& dbsettings = *it;
    CheckIsObject(dbsettings, "dbsettings");
    auto insertion_result = mongo_settings_.emplace(
        std::move(dbalias), GetString(dbsettings, "uri"));
    if (!insertion_result.second) {
      throw InvalidSecdistJson("Duplicate mongo dbalias: '" +
                               insertion_result.first->first + '\'');
    }
  }
}

void SecdistConfig::LoadRedisSettings(const Json::Value& doc) {
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
