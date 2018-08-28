#pragma once

#include <chrono>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include <json/value.h>
#include <redis/secdist_redis.hpp>

namespace storages {
namespace secdist {

class SecdistError : public std::runtime_error {
  using std::runtime_error::runtime_error;
};

class InvalidSecdistJson : public SecdistError {
  using SecdistError::SecdistError;
};

class UnknownMongoDbAlias : public SecdistError {
  using SecdistError::SecdistError;
};

class UnknownRedisClientName : public SecdistError {
  using SecdistError::SecdistError;
};

class SecdistConfig {
 public:
  SecdistConfig();
  explicit SecdistConfig(const std::string& path);

  const std::string& GetMongoConnectionString(const std::string& dbalias) const;
  const ::secdist::RedisSettings& GetRedisSettings(
      const std::string& client_name) const;

 private:
  void LoadMongoSettings(const Json::Value& doc);
  void LoadRedisSettings(const Json::Value& doc);

  std::unordered_map<std::string, std::string> mongo_settings_;
  std::unordered_map<std::string, ::secdist::RedisSettings> redis_settings_;
};

}  // namespace secdist
}  // namespace storages
