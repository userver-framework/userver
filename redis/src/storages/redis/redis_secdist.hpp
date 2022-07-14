#pragma once

#include <chrono>
#include <string>
#include <unordered_map>
#include <vector>

#include <userver/storages/redis/impl/secdist_redis.hpp>

#include <userver/formats/json/value.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::secdist {

class RedisMapSettings {
 public:
  explicit RedisMapSettings(const formats::json::Value& doc);

  const USERVER_NAMESPACE::secdist::RedisSettings& GetSettings(
      const std::string& client_name) const;

 private:
  std::unordered_map<std::string, USERVER_NAMESPACE::secdist::RedisSettings>
      redis_settings_;
};

}  // namespace storages::secdist

USERVER_NAMESPACE_END
