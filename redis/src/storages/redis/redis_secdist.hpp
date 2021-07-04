#pragma once

#include <chrono>
#include <string>
#include <unordered_map>
#include <vector>

#include <userver/storages/redis/impl/secdist_redis.hpp>

#include <userver/formats/json/value.hpp>

namespace storages {
namespace secdist {

class RedisMapSettings {
 public:
  explicit RedisMapSettings(const formats::json::Value& doc);

  const ::secdist::RedisSettings& GetSettings(
      const std::string& client_name) const;

 private:
  std::unordered_map<std::string, ::secdist::RedisSettings> redis_settings_;
};

}  // namespace secdist
}  // namespace storages
