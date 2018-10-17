#include <storages/redis/component.hpp>

#include <stdexcept>
#include <vector>

#include <components/statistics_storage.hpp>
#include <formats/json/value_builder.hpp>
#include <logging/log.hpp>
#include <redis/reply.hpp>
#include <redis/sentinel.hpp>
#include <redis/thread_pools.hpp>
#include <storages/secdist/component.hpp>
#include <storages/secdist/exceptions.hpp>
#include <storages/secdist/secdist.hpp>
#include <utils/statistics.hpp>
#include <utils/statistics/percentile_format_json.hpp>
#include <yaml_config/value.hpp>

#include "redis_config.hpp"
#include "redis_secdist.hpp"

namespace {

const auto kStatisticsName = "redis";

formats::json::ValueBuilder InstanceStatisticsToJson(
    const redis::InstanceStatistics& stats, bool real_instance) {
  formats::json::ValueBuilder result(formats::json::Type::kObject),
      errors(formats::json::Type::kObject),
      states(formats::json::Type::kObject);

  result["request-sizes"]["1min"] =
      utils::statistics::PercentileToJson(stats.request_size_percentile);
  result["reply-sizes"]["1min"] =
      utils::statistics::PercentileToJson(stats.reply_size_percentile);
  result["timings"]["1min"] =
      utils::statistics::PercentileToJson(stats.timings_percentile);

  result["reconnects"] = stats.reconnects;

  for (size_t i = 0; i <= redis::REDIS_ERR_MAX; ++i)
    errors[redis::Reply::StatusToString(i)] = stats.error_count[i];
  result["errors"] = errors;

  if (real_instance) {
    result["last_ping_ms"] = stats.last_ping_ms;

    for (size_t i = 0;
         i <= static_cast<int>(redis::Redis::State::kDisconnectError); ++i) {
      auto state = static_cast<redis::Redis::State>(i);
      states[redis::Redis::StateToString(state)] = stats.state == state ? 1 : 0;
    }
    result["state"] = states;

    long long session_time_ms =
        stats.state == redis::Redis::State::kConnected
            ? (redis::MillisecondsSinceEpoch() - stats.session_start_time)
                  .count()
            : 0;
    result["session-time-ms"] = session_time_ms;
  }

  return result;
}

formats::json::ValueBuilder ShardStatisticsToJson(
    const redis::ShardStatistics& shard_stats) {
  formats::json::ValueBuilder result(formats::json::Type::kObject),
      insts(formats::json::Type::kObject);
  for (const auto& it2 : shard_stats.instances) {
    const auto& inst_name = it2.first;
    const auto& inst_stats = it2.second;
    insts[inst_name] = InstanceStatisticsToJson(inst_stats, true);
  }
  result["instances"] = insts;
  result["instances_count"] = insts.GetSize();

  result["shard-total"] =
      InstanceStatisticsToJson(shard_stats.GetShardTotalStatistics(), false);

  result["is_ready"] = shard_stats.is_ready ? 1 : 0;

  long long not_ready =
      std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::steady_clock::now() - shard_stats.last_ready_time)
          .count();
  result["not_ready_ms"] = shard_stats.is_ready ? 0 : not_ready;
  return result;
}

formats::json::ValueBuilder RedisStatisticsToJson(
    const std::shared_ptr<redis::Sentinel>& redis) {
  formats::json::ValueBuilder result(formats::json::Type::kObject),
      masters(formats::json::Type::kObject),
      slaves(formats::json::Type::kObject);
  auto stats = redis->GetStatistics();

  for (const auto& it : stats.masters) {
    const auto& shard_name = it.first;
    const auto& shard_stats = it.second;
    masters[shard_name] = ShardStatisticsToJson(shard_stats);
  }
  result["masters"] = masters;
  for (const auto& it : stats.slaves) {
    const auto& shard_name = it.first;
    const auto& shard_stats = it.second;
    slaves[shard_name] = ShardStatisticsToJson(shard_stats);
  }
  result["slaves"] = slaves;
  result["sentinels"] = ShardStatisticsToJson(stats.sentinel);

  result["shard-group-total"] =
      InstanceStatisticsToJson(stats.GetShardGroupTotalStatistics(), false);

  result["errors"] = formats::json::Type::kObject;
  result["errors"]["redis_not_ready"] = stats.internal.redis_not_ready.load();
  return result;
}

}  // namespace

namespace components {

struct RedisGroup {
  std::string db;
  std::string config_name;
  std::string sharding_strategy;

  static RedisGroup ParseFromYaml(
      const formats::yaml::Node& yaml, const std::string& full_path,
      const yaml_config::VariableMapPtr& config_vars_ptr) {
    RedisGroup config;
    config.db =
        yaml_config::ParseString(yaml, "db", full_path, config_vars_ptr);
    config.config_name = yaml_config::ParseString(yaml, "config_name",
                                                  full_path, config_vars_ptr);
    config.sharding_strategy =
        yaml_config::ParseOptionalString(yaml, "sharding_strategy", full_path,
                                         config_vars_ptr)
            .value_or("");
    return config;
  }
};

struct RedisPools {
  int sentinel_thread_pool_size;
  int redis_thread_pool_size;

  static RedisPools ParseFromYaml(
      const formats::yaml::Node& yaml, const std::string& full_path,
      const yaml_config::VariableMapPtr& config_vars_ptr) {
    RedisPools pools;
    pools.sentinel_thread_pool_size = yaml_config::ParseInt(
        yaml, "sentinel_thread_pool_size", full_path, config_vars_ptr);
    pools.redis_thread_pool_size = yaml_config::ParseInt(
        yaml, "redis_thread_pool_size", full_path, config_vars_ptr);
    return pools;
  }
};

Redis::Redis(const ComponentConfig& config,
             const ComponentContext& component_context)
    : config_{component_context.FindComponent<TaxiConfig>()} {
  Connect(config, component_context);

  config_subscription_ = config_->AddListener(this, &Redis::OnConfigUpdate);
  OnConfigUpdate(config_->Get());

  statistics_storage_ =
      component_context.FindComponentRequired<components::StatisticsStorage>();

  statistics_holder_ = statistics_storage_->GetStorage().RegisterExtender(
      kStatisticsName,
      std::bind(&Redis::ExtendStatistics, this, std::placeholders::_1));
}

std::shared_ptr<redis::Sentinel> Redis::Client(const std::string& name) const {
  auto it = clients_.find(name);
  if (it == clients_.end())
    throw std::runtime_error(name + " redis client not found");
  return it->second;
}

void Redis::Connect(const ComponentConfig& config,
                    const ComponentContext& component_context) {
  if (!config_)
    throw std::runtime_error("Redis component requires taxi config");
  auto secdist_component = component_context.FindComponent<Secdist>();
  if (!secdist_component) {
    throw std::runtime_error("secdist component not found");
  }

  const RedisPools& redis_pools = RedisPools::ParseFromYaml(
      config.Yaml()["thread_pools"], config.FullPath() + ".thread_pools",
      config.ConfigVarsPtr());

  thread_pools_ = std::make_shared<redis::ThreadPools>(
      redis_pools.sentinel_thread_pool_size,
      redis_pools.redis_thread_pool_size);

  std::vector<RedisGroup> redis_groups = yaml_config::ParseArray<RedisGroup>(
      config.Yaml(), "groups", config.FullPath(), config.ConfigVarsPtr());
  for (const RedisGroup& redis_group : redis_groups) {
    std::shared_ptr<redis::Sentinel> client;
    ::secdist::RedisSettings settings;
    const auto& shard_group_name = redis_group.config_name;

    try {
      settings = secdist_component->Get()
                     .Get<storages::secdist::RedisMapSettings>()
                     .GetSettings(redis_group.config_name);
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

Redis::~Redis() {
  try {
    statistics_holder_.Unregister();
    config_subscription_.Unsubscribe();
  } catch (std::exception const& e) {
    LOG_ERROR() << "exception while destroying Redis component: " << e.what();
  } catch (...) {
    LOG_ERROR() << "non-standard exception while destroying Redis component";
  }
}

formats::json::Value Redis::ExtendStatistics(
    const utils::statistics::StatisticsRequest& /*request*/) {
  formats::json::ValueBuilder json(formats::json::Type::kObject);

  for (const auto& client : clients_) {
    const auto& name = client.first;
    const auto& redis = client.second;
    json[name] = RedisStatisticsToJson(redis);
  }
  return json.ExtractValue();
}

void Redis::OnConfigUpdate(const TaxiConfigPtr& cfg) {
  auto cc = std::make_shared<redis::CommandControl>(
      cfg->Get<storages::redis::Config>().default_command_control);

  LOG_INFO() << "update default command control";
  for (auto& it : clients_) {
    auto& client = it.second;
    client->SetConfigDefaultCommandControl(cc);
  }
}

}  // namespace components
