#pragma once

#include <chrono>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include <json/value.h>

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

struct RedisSettings {
  struct HostPort {
    std::string host;
    uint16_t port = 0;
  };

  struct CommandControl {
    std::chrono::milliseconds timeout_single{0};
    std::chrono::milliseconds timeout_all{0};
    int max_retries = 0;
  };

  std::vector<std::string> shards;
  std::vector<HostPort> sentinels;
  std::string password;
  CommandControl command_control;
};

class SecdistConfig {
 public:
  SecdistConfig();
  explicit SecdistConfig(const std::string& path);

  const std::string& GetMongoConnectionString(const std::string& dbalias) const;
  const RedisSettings& GetRedisSettings(const std::string& client_name) const;

 private:
  void LoadMongoSettings(const Json::Value& doc);
  void LoadRedisSettings(const Json::Value& doc);

  std::unordered_map<std::string, std::string> mongo_settings_;
  std::unordered_map<std::string, RedisSettings> redis_settings_;
};

}  // namespace secdist
}  // namespace storages
