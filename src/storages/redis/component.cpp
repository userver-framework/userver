#include "component.hpp"

#include <vector>

#include <json_config/value.hpp>
#include <logging/log.hpp>
#include <redis/sentinel.hpp>
#include <redis/thread_pools.hpp>
#include <storages/secdist/component.hpp>
#include <storages/secdist/secdist.hpp>
#include <yandex/taxi/userver/components/thread_pool.hpp>
#include <yandex/taxi/userver/storages/secdist/component.hpp>
#include <yandex/taxi/userver/storages/secdist/secdist.hpp>

namespace components {

struct RedisGroup {
  std::string db;
  std::string config_name;
  std::string sharding_strategy;

  static RedisGroup ParseFromJson(
      const Json::Value& json, const std::string& full_path,
      const json_config::VariableMapPtr& config_vars_ptr) {
    RedisGroup config;
    config.db =
        json_config::ParseString(json, "db", full_path, config_vars_ptr);
    config.config_name = json_config::ParseString(json, "config_name",
                                                  full_path, config_vars_ptr);
    config.sharding_strategy =
        json_config::ParseOptionalString(json, "sharding_strategy", full_path,
                                         config_vars_ptr)
            .value_or("");
    return config;
  }
};

struct RedisPools {
  int sentinel_thread_pool_size;
  int redis_thread_pool_size;

  static RedisPools ParseFromJson(
      const Json::Value& json, const std::string& full_path,
      const json_config::VariableMapPtr& config_vars_ptr) {
    RedisPools pools;
    pools.sentinel_thread_pool_size = json_config::ParseInt(
        json, "sentinel_thread_pool_size", full_path, config_vars_ptr);
    pools.redis_thread_pool_size = json_config::ParseInt(
        json, "redis_thread_pool_size", full_path, config_vars_ptr);
    return pools;
  }
};

Redis::Redis(const ComponentConfig& config,
             const ComponentContext& component_context) {
  auto secdist_component = component_context.FindComponent<Secdist>();
  if (!secdist_component) {
    throw std::runtime_error("secdist component not found");
  }

  const RedisPools& redis_pools = RedisPools::ParseFromJson(
      config.Json()["thread_pools"], config.FullPath() + ".thread_pools",
      config.ConfigVarsPtr());

  thread_pools_ = std::make_shared<redis::ThreadPools>(
      redis_pools.sentinel_thread_pool_size,
      redis_pools.redis_thread_pool_size);

  std::vector<RedisGroup> redis_groups = json_config::ParseArray<RedisGroup>(
      config.Json(), "groups", config.FullPath(), config.ConfigVarsPtr());
  for (const RedisGroup& redis_group : redis_groups) {
    std::shared_ptr<redis::Sentinel> client;
    secdist::RedisSettings settings;
    const auto& shard_group_name = redis_group.config_name;

    try {
      settings = secdist_component->Get().GetRedisSettings(shard_group_name);
    } catch (const storages::secdist::SecdistError& ex) {
      LOG_ERROR() << "Failed to load redis config (db=" << redis_group.db
                  << " config_name=" << redis_group.config_name
                  << "): " << ex.what();
      throw;
    }
    client = redis::Sentinel::CreateSentinel(
        thread_pools_, settings, shard_group_name, redis_group.db,
        redis::KeyShardFactory{redis_group.sharding_strategy});
    if (client)
      clients_.emplace(redis_group.db, client);
    else
      LOG_WARNING() << "skip redis client for " << redis_group.db;
  }
}

Redis::~Redis() = default;

}  // namespace components
