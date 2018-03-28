#include "component.hpp"

#include <vector>

#include <components/thread_pool.hpp>
#include <json_config/value.hpp>
#include <logging/logger.hpp>
#include <storages/redis/sentinel.hpp>
#include <storages/secdist/component.hpp>
#include <storages/secdist/secdist.hpp>

namespace components {

struct RedisGroup {
  std::string db;
  std::string config_name;

  static RedisGroup ParseFromJson(
      const Json::Value& json, const std::string& full_path,
      const json_config::VariableMapPtr& config_vars_ptr) {
    RedisGroup config;
    config.db =
        json_config::ParseString(json, "db", full_path, config_vars_ptr);
    config.config_name = json_config::ParseString(json, "config_name",
                                                  full_path, config_vars_ptr);
    return config;
  }
};

Redis::Redis(const ComponentConfig& config,
             const ComponentContext& component_context) {
  std::string thread_pool_name = config.ParseString("thread_pool");
  auto thread_pool_component =
      component_context.FindComponent<ThreadPool>(thread_pool_name);
  if (!thread_pool_component) {
    throw std::runtime_error("thread pool component '" + thread_pool_name +
                             "' not found");
  }

  std::string sentinel_thread_pool_name =
      config.ParseString("sentinel_thread_pool");
  auto sentinel_thread_pool_component =
      component_context.FindComponent<ThreadPool>(sentinel_thread_pool_name);
  if (!sentinel_thread_pool_component) {
    throw std::runtime_error("sentinel thread pool component '" +
                             sentinel_thread_pool_name + "' not found");
  }

  auto secdist_component = component_context.FindComponent<Secdist>();
  if (!secdist_component) {
    throw std::runtime_error("secdist component not found");
  }

  std::string task_processor_name = config.ParseString("task_processor");
  engine::TaskProcessor* task_processor =
      component_context.GetTaskProcessor(task_processor_name);
  if (!task_processor) {
    throw std::runtime_error("can't find task_processor '" +
                             task_processor_name + "'");
  }

  std::vector<RedisGroup> redis_groups = json_config::ParseArray<RedisGroup>(
      config.Json(), "groups", config.FullPath(), config.ConfigVarsPtr());
  for (const RedisGroup& redis_group : redis_groups) {
    std::shared_ptr<storages::redis::Sentinel> client;
    storages::secdist::RedisSettings settings;
    try {
      settings =
          secdist_component->Get().GetRedisSettings(redis_group.config_name);
    } catch (const storages::secdist::SecdistError& ex) {
      LOG_ERROR() << "Failed to load redis config (db=" << redis_group.db
                  << " config_name=" << redis_group.config_name
                  << "): " << ex.what();
      throw;
    }
    client = storages::redis::Sentinel::CreateSentinel(
        settings, sentinel_thread_pool_component->Get().NextThread(),
        thread_pool_component->Get(), *task_processor);
    if (client)
      clients_.emplace(redis_group.db, client);
    else
      LOG_WARNING() << "skip redis client for " << redis_group.db;
  }
}

}  // namespace components
