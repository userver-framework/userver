#include <storages/redis/component.hpp>

#include <stdexcept>
#include <vector>

#include <components/statistics_storage.hpp>
#include <formats/json/value_builder.hpp>
#include <logging/log.hpp>
#include <redis/reply.hpp>
#include <redis/sentinel.hpp>
#include <redis/subscribe_sentinel.hpp>
#include <redis/thread_pools.hpp>
#include <storages/secdist/component.hpp>
#include <storages/secdist/exceptions.hpp>
#include <storages/secdist/secdist.hpp>
#include <utils/statistics.hpp>
#include <utils/statistics/percentile_format_json.hpp>
#include <yaml_config/value.hpp>

#include <storages/redis/redis_config.hpp>
#include <storages/redis/subscribe_client.hpp>
#include "redis_secdist.hpp"

namespace {

const auto kStatisticsName = "redis";
const auto kSubscribeStatisticsName = "redis-pubsub";

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

formats::json::ValueBuilder PubsubChannelStatisticsToJson(
    const redis::PubsubChannelStatistics& stats, bool extra) {
  formats::json::ValueBuilder json(formats::json::Type::kObject);
  json["messages"]["count"] = stats.messages_count;
  json["messages"]["alien-count"] = stats.messages_alien_count;
  json["messages"]["size"] = stats.messages_size;

  if (extra) {
    auto diff = std::chrono::steady_clock::now() - stats.subscription_timestamp;
    json["subscribed-ms"] =
        std::chrono::duration_cast<std::chrono::milliseconds>(diff).count();

    auto inst_name = stats.server_id.GetDescription();
    if (inst_name.empty()) inst_name = "unknown";
    json["instances"][inst_name] = 1;
  }
  return json;
}

formats::json::ValueBuilder PubsubShardStatisticsToJson(
    const redis::PubsubShardStatistics& stats, bool extra) {
  formats::json::ValueBuilder json(formats::json::Type::kObject);
  for (auto& it : stats.by_channel) {
    json[it.first] = PubsubChannelStatisticsToJson(it.second, extra);
  }
  return json;
}

formats::json::ValueBuilder RedisSubscribeStatisticsToJson(
    const redis::SubscribeSentinel& redis) {
  auto stats = redis.GetSubscriberStatistics();
  formats::json::ValueBuilder result(formats::json::Type::kObject);

  formats::json::ValueBuilder by_shard(formats::json::Type::kObject);
  for (auto& it : stats.by_shard) {
    by_shard[it.first] = PubsubShardStatisticsToJson(it.second, true);
  }
  result["by-shard"] = std::move(by_shard);

  auto total_stats = stats.SumByShards();
  result["shard-group-total"] = PubsubShardStatisticsToJson(total_stats, false);

  return result;
}

template <typename RedisGroup>
::secdist::RedisSettings GetSecdistSettings(
    components::Secdist& secdist_component, const RedisGroup& redis_group) {
  try {
    return secdist_component.Get()
        .Get<storages::secdist::RedisMapSettings>()
        .GetSettings(redis_group.config_name);
  } catch (const storages::secdist::SecdistError& ex) {
    LOG_ERROR() << "Failed to load redis config (db=" << redis_group.db
                << " config_name=" << redis_group.config_name << "): " << ex;
    throw;
  }
}

}  // namespace

namespace components {

struct RedisGroup {
  std::string db;
  std::string config_name;
  std::string sharding_strategy;

  static RedisGroup ParseFromYaml(
      const formats::yaml::Value& yaml, const std::string& full_path,
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

struct SubscribeRedisGroup {
  std::string db;
  std::string config_name;

  static SubscribeRedisGroup ParseFromYaml(
      const formats::yaml::Value& yaml, const std::string& full_path,
      const yaml_config::VariableMapPtr& config_vars_ptr) {
    SubscribeRedisGroup config;
    config.db =
        yaml_config::ParseString(yaml, "db", full_path, config_vars_ptr);
    config.config_name = yaml_config::ParseString(yaml, "config_name",
                                                  full_path, config_vars_ptr);
    return config;
  }
};

struct RedisPools {
  int sentinel_thread_pool_size;
  int redis_thread_pool_size;

  static RedisPools ParseFromYaml(
      const formats::yaml::Value& yaml, const std::string& full_path,
      const yaml_config::VariableMapPtr& config_vars_ptr) {
    RedisPools pools{};
    pools.sentinel_thread_pool_size = yaml_config::ParseInt(
        yaml, "sentinel_thread_pool_size", full_path, config_vars_ptr);
    pools.redis_thread_pool_size = yaml_config::ParseInt(
        yaml, "redis_thread_pool_size", full_path, config_vars_ptr);
    return pools;
  }
};

Redis::Redis(const ComponentConfig& config,
             const ComponentContext& component_context)
    : LoggableComponentBase(config, component_context),
      config_(component_context.FindComponent<TaxiConfig>()),
      statistics_storage_(
          component_context.FindComponent<components::StatisticsStorage>()) {
  Connect(config, component_context);

  OnConfigUpdate(config_.Get());
  config_subscription_ =
      config_.AddListener(this, "redis", &Redis::OnConfigUpdate);

  statistics_holder_ = statistics_storage_.GetStorage().RegisterExtender(
      kStatisticsName,
      std::bind(&Redis::ExtendStatisticsRedis, this, std::placeholders::_1));

  subscribe_statistics_holder_ =
      statistics_storage_.GetStorage().RegisterExtender(
          kSubscribeStatisticsName,
          std::bind(&Redis::ExtendStatisticsRedisPubsub, this,
                    std::placeholders::_1));
}

std::shared_ptr<redis::Sentinel> Redis::Client(const std::string& name) const {
  auto it = clients_.find(name);
  if (it == clients_.end())
    throw std::runtime_error(name + " redis client not found");
  return it->second;
}

std::shared_ptr<storages::redis::SubscribeClient> Redis::GetSubscribeClient(
    const std::string& name) const {
  auto it = subscribe_clients_.find(name);
  if (it == subscribe_clients_.end())
    throw std::runtime_error(name + " redis subscribe-client not found");
  return it->second;
}

void Redis::Connect(const ComponentConfig& config,
                    const ComponentContext& component_context) {
  auto& secdist_component = component_context.FindComponent<Secdist>();

  const RedisPools& redis_pools = RedisPools::ParseFromYaml(
      config.Yaml()["thread_pools"], config.FullPath() + ".thread_pools",
      config.ConfigVarsPtr());

  thread_pools_ = std::make_shared<redis::ThreadPools>(
      redis_pools.sentinel_thread_pool_size,
      redis_pools.redis_thread_pool_size);

  auto redis_groups = yaml_config::Parse<std::vector<RedisGroup>>(
      config.Yaml(), "groups", config.FullPath(), config.ConfigVarsPtr());
  for (const RedisGroup& redis_group : redis_groups) {
    auto settings = GetSecdistSettings(secdist_component, redis_group);

    auto client = redis::Sentinel::CreateSentinel(
        thread_pools_, settings, redis_group.config_name, redis_group.db,
        redis::KeyShardFactory{redis_group.sharding_strategy});
    if (client)
      clients_.emplace(redis_group.db, client);
    else
      LOG_WARNING() << "skip redis client for " << redis_group.db;
  }

  auto subscribe_redis_groups =
      yaml_config::Parse<std::vector<SubscribeRedisGroup>>(
          config.Yaml(), "subscribe_groups", config.FullPath(),
          config.ConfigVarsPtr());
  for (const auto& redis_group : subscribe_redis_groups) {
    auto settings = GetSecdistSettings(secdist_component, redis_group);

    auto client = redis::SubscribeSentinel::Create(
        thread_pools_, settings, redis_group.config_name, redis_group.db);
    if (client)
      subscribe_clients_.emplace(
          redis_group.db, std::make_shared<storages::redis::SubscribeClient>(
                              std::move(client)));
    else
      LOG_WARNING() << "skip subscribe-redis client for " << redis_group.db;
  }
}

Redis::~Redis() {
  try {
    statistics_holder_.Unregister();
    subscribe_statistics_holder_.Unregister();
    config_subscription_.Unsubscribe();
  } catch (std::exception const& e) {
    LOG_ERROR() << "exception while destroying Redis component: " << e;
  } catch (...) {
    LOG_ERROR() << "non-standard exception while destroying Redis component";
  }
}

formats::json::Value Redis::ExtendStatisticsRedis(
    const utils::statistics::StatisticsRequest& /*request*/) {
  formats::json::ValueBuilder json(formats::json::Type::kObject);
  for (const auto& client : clients_) {
    const auto& name = client.first;
    const auto& redis = client.second;
    json[name] = RedisStatisticsToJson(redis);
  }
  return json.ExtractValue();
}

formats::json::Value Redis::ExtendStatisticsRedisPubsub(
    const utils::statistics::StatisticsRequest& /*request*/) {
  formats::json::ValueBuilder subscribe_json(formats::json::Type::kObject);
  for (const auto& client : subscribe_clients_) {
    const auto& name = client.first;
    const auto& redis = client.second->GetNative();
    subscribe_json[name] = RedisSubscribeStatisticsToJson(redis);
  }
  return subscribe_json.ExtractValue();
}

void Redis::OnConfigUpdate(const TaxiConfigPtr& cfg) {
  const auto& redis_config = cfg->Get<storages::redis::Config>();
  auto cc = std::make_shared<redis::CommandControl>(
      redis_config.default_command_control);
  std::chrono::seconds subscriptions_rebalance_min_interval{
      redis_config.subscriptions_rebalance_min_interval_seconds.Get()};

  LOG_INFO() << "update default command control";
  for (auto& it : clients_) {
    auto& client = it.second;
    client->SetConfigDefaultCommandControl(cc);
  }

  for (auto& it : subscribe_clients_) {
    auto& subscribe_client = it.second->GetNative();
    subscribe_client.SetConfigDefaultCommandControl(cc);
    subscribe_client.SetRebalanceMinInterval(
        subscriptions_rebalance_min_interval);
  }
}

}  // namespace components
