#include <userver/storages/redis/component.hpp>

#include <stdexcept>
#include <vector>

#include <engine/ev/thread_pool.hpp>
#include <userver/clients/dns/resolver_utils.hpp>
#include <userver/components/component.hpp>
#include <userver/components/statistics_storage.hpp>
#include <userver/dynamic_config/storage/component.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/logging/log.hpp>
#include <userver/storages/redis/impl/thread_pools.hpp>
#include <userver/storages/redis/reply.hpp>
#include <userver/storages/secdist/component.hpp>
#include <userver/storages/secdist/exceptions.hpp>
#include <userver/storages/secdist/secdist.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/utils/statistics/writer.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

#include <userver/storages/redis/client.hpp>
#include <userver/storages/redis/redis_config.hpp>
#include <userver/storages/redis/subscribe_client.hpp>

#include <storages/redis/impl/keyshard_impl.hpp>
#include <storages/redis/impl/sentinel.hpp>
#include <storages/redis/impl/subscribe_sentinel.hpp>

#include "client_impl.hpp"
#include "redis_secdist.hpp"
#include "subscribe_client_impl.hpp"

USERVER_NAMESPACE_BEGIN

namespace {

const auto kStatisticsName = "redis";
const auto kSubscribeStatisticsName = "redis-pubsub";

void DumpThreadPoolMetric(utils::statistics::Writer& writer,
                          engine::ev::ThreadPool& pool) {
  for (const auto* thread : pool.NextThreads(pool.GetSize())) {
    writer.ValueWithLabels(thread->GetCurrentLoadPercent(),
                           {"ev_thread_name", thread->GetName()});
  }
}

template <typename RedisGroup>
USERVER_NAMESPACE::secdist::RedisSettings GetSecdistSettings(
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
  bool allow_reads_from_master{false};
};

RedisGroup Parse(const yaml_config::YamlConfig& value,
                 formats::parse::To<RedisGroup>) {
  RedisGroup config;
  config.db = value["db"].As<std::string>();
  config.config_name = value["config_name"].As<std::string>();
  config.sharding_strategy = value["sharding_strategy"].As<std::string>("");
  config.allow_reads_from_master =
      value["allow_reads_from_master"].As<bool>(false);
  return config;
}

struct SubscribeRedisGroup {
  std::string db;
  std::string config_name;
  std::string sharding_strategy;
};

SubscribeRedisGroup Parse(const yaml_config::YamlConfig& value,
                          formats::parse::To<SubscribeRedisGroup>) {
  SubscribeRedisGroup config;
  config.db = value["db"].As<std::string>();
  config.config_name = value["config_name"].As<std::string>();
  config.sharding_strategy = value["sharding_strategy"].As<std::string>("");
  return config;
}

struct RedisPools {
  int sentinel_thread_pool_size;
  int redis_thread_pool_size;
};

RedisPools Parse(const yaml_config::YamlConfig& value,
                 formats::parse::To<RedisPools>) {
  RedisPools pools{};
  pools.sentinel_thread_pool_size =
      value["sentinel_thread_pool_size"].As<int>();
  pools.redis_thread_pool_size = value["redis_thread_pool_size"].As<int>();
  return pools;
}

Redis::Redis(const ComponentConfig& config,
             const ComponentContext& component_context)
    : LoggableComponentBase(config, component_context),
      config_(component_context.FindComponent<DynamicConfig>().GetSource()) {
  const auto& testsuite_redis_control =
      component_context.FindComponent<components::TestsuiteSupport>()
          .GetRedisControl();
  Connect(config, component_context, testsuite_redis_control);

  config_subscription_ =
      config_.UpdateAndListen(this, "redis", &Redis::OnConfigUpdate);

  auto& statistics_storage =
      component_context.FindComponent<components::StatisticsStorage>()
          .GetStorage();

  statistics_holder_ = statistics_storage.RegisterWriter(
      kStatisticsName,
      [this](utils::statistics::Writer& writer) { WriteStatistics(writer); });

  subscribe_statistics_holder_ = statistics_storage.RegisterWriter(
      kSubscribeStatisticsName, [this](utils::statistics::Writer& writer) {
        WriteStatisticsPubsub(writer);
      });
}

std::shared_ptr<storages::redis::Client> Redis::GetClient(
    const std::string& name,
    USERVER_NAMESPACE::redis::RedisWaitConnected wait_connected) const {
  auto it = clients_.find(name);
  if (it == clients_.end())
    throw std::runtime_error(name + " redis client not found");
  it->second->WaitConnectedOnce(wait_connected);
  return it->second;
}

std::shared_ptr<redis::Sentinel> Redis::Client(const std::string& name) const {
  auto it = sentinels_.find(name);
  if (it == sentinels_.end())
    throw std::runtime_error(name + " redis client not found");
  return it->second;
}

std::shared_ptr<storages::redis::SubscribeClient> Redis::GetSubscribeClient(
    const std::string& name,
    USERVER_NAMESPACE::redis::RedisWaitConnected wait_connected) const {
  auto it = subscribe_clients_.find(name);
  if (it == subscribe_clients_.end())
    throw std::runtime_error(name + " redis subscribe-client not found");
  it->second->WaitConnectedOnce(wait_connected);
  return std::static_pointer_cast<storages::redis::SubscribeClient>(it->second);
}

void Redis::Connect(const ComponentConfig& config,
                    const ComponentContext& component_context,
                    const testsuite::RedisControl& testsuite_redis_control) {
  auto& secdist_component = component_context.FindComponent<Secdist>();

  auto config_source =
      component_context.FindComponent<DynamicConfig>().GetSource();

  const auto redis_pools = config["thread_pools"].As<RedisPools>();

  thread_pools_ = std::make_shared<redis::ThreadPools>(
      redis_pools.sentinel_thread_pool_size,
      redis_pools.redis_thread_pool_size);

  clients::dns::Resolver* dns_resolver =
      clients::dns::GetResolverPtr(config, component_context);

  const auto redis_groups = config["groups"].As<std::vector<RedisGroup>>();
  for (const RedisGroup& redis_group : redis_groups) {
    auto settings = GetSecdistSettings(secdist_component, redis_group);

    auto command_control = redis::kDefaultCommandControl;
    command_control.allow_reads_from_master =
        redis_group.allow_reads_from_master;

    auto sentinel = redis::Sentinel::CreateSentinel(
        thread_pools_, settings, redis_group.config_name, config_source,
        redis_group.db, redis::KeyShardFactory{redis_group.sharding_strategy},
        command_control, testsuite_redis_control, dns_resolver);
    if (sentinel) {
      sentinels_.emplace(redis_group.db, sentinel);
      const auto& client =
          std::make_shared<storages::redis::ClientImpl>(sentinel);
      clients_.emplace(redis_group.db, client);
    } else {
      LOG_WARNING() << "skip redis client for " << redis_group.db;
    }
  }

  auto cfg = config_.GetSnapshot();
  const auto& redis_config = cfg.Get<storages::redis::Config>();
  for (auto& sentinel_it : sentinels_) {
    sentinel_it.second->WaitConnectedOnce(redis_config.redis_wait_connected);
  }

  auto subscribe_redis_groups =
      config["subscribe_groups"].As<std::vector<SubscribeRedisGroup>>();

  for (const auto& redis_group : subscribe_redis_groups) {
    auto settings = GetSecdistSettings(secdist_component, redis_group);

    bool is_cluster_mode = USERVER_NAMESPACE::redis::IsClusterStrategy(
        redis_group.sharding_strategy);

    auto sentinel = redis::SubscribeSentinel::Create(
        thread_pools_, settings, redis_group.config_name, config_source,
        redis_group.db, is_cluster_mode, testsuite_redis_control, dns_resolver);
    if (sentinel)
      subscribe_clients_.emplace(
          redis_group.db,
          std::make_shared<storages::redis::SubscribeClientImpl>(
              std::move(sentinel)));
    else
      LOG_WARNING() << "skip subscribe-redis client for " << redis_group.db;
  }

  auto redis_wait_connected_subscribe = redis_config.redis_wait_connected.Get();
  if (redis_wait_connected_subscribe.mode !=
      USERVER_NAMESPACE::redis::WaitConnectedMode::kNoWait)
    redis_wait_connected_subscribe.mode =
        USERVER_NAMESPACE::redis::WaitConnectedMode::kMasterOrSlave;
  for (auto& subscribe_client_it : subscribe_clients_) {
    subscribe_client_it.second->WaitConnectedOnce(
        redis_wait_connected_subscribe);
  }
}

Redis::~Redis() {
  statistics_holder_.Unregister();
  subscribe_statistics_holder_.Unregister();
  config_subscription_.Unsubscribe();
}

void Redis::WriteStatistics(utils::statistics::Writer& writer) {
  auto settings = metrics_settings_.Read();
  for (const auto& [name, redis] : sentinels_) {
    writer.ValueWithLabels(redis->GetStatistics(*settings),
                           {"redis_database", name});
  }
  auto threads_writer = writer["ev_threads"]["cpu_load_percent"];
  DumpThreadPoolMetric(threads_writer, *thread_pools_->GetRedisThreadPool());
  DumpThreadPoolMetric(threads_writer, thread_pools_->GetSentinelThreadPool());
}

void Redis::WriteStatisticsPubsub(utils::statistics::Writer& writer) {
  auto settings = pubsub_metrics_settings_.Read();
  for (const auto& [name, redis] : subscribe_clients_) {
    writer.ValueWithLabels(
        redis->GetNative().GetSubscriberStatistics(*settings),
        {"redis_database", name});
  }
}

void Redis::OnConfigUpdate(const dynamic_config::Snapshot& cfg) {
  LOG_INFO() << "update default command control";
  const auto& redis_config = cfg.Get<storages::redis::Config>();

  auto cc = std::make_shared<redis::CommandControl>(
      redis_config.default_command_control);
  const auto auto_topology =
      redis_config.redis_cluster_autotopology_enabled.Get();
  for (auto& it : sentinels_) {
    const auto& name = it.first;
    auto& client = it.second;
    client->SetConfigDefaultCommandControl(cc);
    client->SetCommandsBufferingSettings(
        redis_config.commands_buffering_settings);
    client->SetReplicationMonitoringSettings(
        redis_config.replication_monitoring_settings.GetOptional(name).value_or(
            redis::ReplicationMonitoringSettings{}));
    client->SetClusterAutoTopology(auto_topology);
  }

  auto subscriber_cc = std::make_shared<redis::CommandControl>(
      redis_config.subscriber_default_command_control);
  std::chrono::seconds subscriptions_rebalance_min_interval{
      redis_config.subscriptions_rebalance_min_interval_seconds.Get()};
  for (auto& it : subscribe_clients_) {
    auto& subscribe_client = it.second->GetNative();
    subscribe_client.SetConfigDefaultCommandControl(subscriber_cc);
    subscribe_client.SetRebalanceMinInterval(
        subscriptions_rebalance_min_interval);
  }

  auto metrics_settings = metrics_settings_.Read();
  if (*metrics_settings != redis_config.metrics_settings) {
    metrics_settings_.Assign(redis_config.metrics_settings);
  }

  auto pubsub_metrics_settings = pubsub_metrics_settings_.Read();
  if (*pubsub_metrics_settings != redis_config.pubsub_metrics_settings) {
    pubsub_metrics_settings_.Assign(redis_config.pubsub_metrics_settings);
  }
}

yaml_config::Schema Redis::GetStaticConfigSchema() {
  return yaml_config::MergeSchemas<LoggableComponentBase>(R"(
type: object
description: Redis client component
additionalProperties: false
properties:
    thread_pools:
        type: object
        description: thread pools options
        additionalProperties: false
        properties:
            redis_thread_pool_size:
                type: integer
                description: thread count to serve Redis requests
            sentinel_thread_pool_size:
                type: integer
                description: thread count to serve sentinel requests
    groups:
        type: array
        description: array of redis clusters to work with excluding subscribers
        items:
            type: object
            description: redis cluster to work with excluding subscribers
            additionalProperties: false
            properties:
                config_name:
                    type: string
                    description: key name in secdist with options for this cluster
                db:
                    type: string
                    description: name to refer to the cluster in components::Redis::GetClient()
                sharding_strategy:
                    type: string
                    description: one of RedisCluster, KeyShardCrc32, KeyShardTaximeterCrc32 or KeyShardGpsStorageDriver
                    defaultDescription: "KeyShardTaximeterCrc32"
                    enum:
                      - RedisCluster
                      - KeyShardCrc32
                      - KeyShardTaximeterCrc32
                      - KeyShardGpsStorageDriver
                allow_reads_from_master:
                    type: boolean
                    description: allows read requests from master instance
                    defaultDescription: false
    subscribe_groups:
        type: array
        description: array of redis clusters to work with in subscribe mode
        items:
            type: object
            description: redis cluster to work with in subscribe mode
            additionalProperties: false
            properties:
                config_name:
                    type: string
                    description: key name in secdist with options for this cluster
                db:
                    type: string
                    description: name to refer to the cluster in components::Redis::GetSubscribeClient()
                sharding_strategy:
                    type: string
                    description: either RedisCluster or KeyShardTaximeterCrc32
                    defaultDescription: "KeyShardTaximeterCrc32"
                    enum:
                      - RedisCluster
                      - KeyShardTaximeterCrc32
)");
}

}  // namespace components

USERVER_NAMESPACE_END
