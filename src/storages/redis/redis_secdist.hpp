#pragma once

#include <chrono>
#include <string>
#include <unordered_map>
#include <vector>

#include <redis/secdist_redis.hpp>

namespace Json {
class Value;
}  // namespace Json

namespace storages {
namespace secdist {

class RedisMapSettings {
 public:
  explicit RedisMapSettings(const Json::Value& doc);

  const ::secdist::RedisSettings& GetSettings(
      const std::string& client_name) const;

 private:
  std::unordered_map<std::string, ::secdist::RedisSettings> redis_settings_;
};

}  // namespace secdist
}  // namespace storages
