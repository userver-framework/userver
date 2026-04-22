#include <userver/storages/redis/dynamic_component.hpp>

#include <stdexcept>

#include <fmt/ranges.h>

#include <engine/ev/thread_pool.hpp>
#include <storages/redis/impl/thread_pools.hpp>
#include <userver/components/component.hpp>
#include <userver/components/statistics_storage.hpp>
#include <userver/dynamic_config/storage/component.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/logging/log.hpp>
#include <userver/storages/redis/reply.hpp>
#include <userver/storages/secdist/component.hpp>
#include <userver/storages/secdist/exceptions.hpp>
#include <userver/storages/secdist/secdist.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/utils/retry_budget.hpp>
#include <userver/utils/statistics/writer.hpp>
#include <userver/utils/trivial_map.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

#include <userver/storages/redis/client.hpp>
#include <userver/storages/redis/redis_config.hpp>
#include <userver/storages/redis/subscribe_client.hpp>

#include <storages/redis/impl/keyshard_impl.hpp>
#include <storages/redis/impl/sentinel.hpp>
#include <storages/redis/impl/subscribe_sentinel.hpp>

#include "redis_secdist.hpp"
#include "userver/storages/redis/base.hpp"
#include "userver/storages/redis/wait_connected_mode.hpp"

#include <boost/range/adaptor/map.hpp>

#ifndef ARCADIA_ROOT
#include "generated/src/storages/redis/dynamic_component.yaml.hpp"  // Y_IGNORE
#endif

USERVER_NAMESPACE_BEGIN

namespace {

const auto kStatisticsName = "redis";

template <typename RedisGroup>
USERVER_NAMESPACE::secdist::RedisSettings GetSecdistSettings(
    components::Secdist& secdist_component,
    const RedisGroup& redis_group
) {
    try {
        return secdist_component.Get().Get<storages::secdist::RedisMapSettings>().GetSettings(redis_group.config_name);
    } catch (const storages::secdist::SecdistError& ex) {
        LOG_ERROR()
            << "Failed to load redis config (db=" << redis_group.db << " config_name=" << redis_group.config_name
            << "): " << ex;
        throw;
    }
}

}  // namespace

namespace components {

struct RedisGroup {
    std::string db;
    std::string config_name;
    storages::redis::ShardingStrategy sharding_strategy{storages::redis::ShardingStrategy::kKeyShardTaximeterCrc32};
    bool allow_reads_from_master{false};
};

RedisGroup Parse(const yaml_config::YamlConfig& value, formats::parse::To<RedisGroup>) {
    RedisGroup config;
    config.db = value["db"].As<std::string>();
    config.config_name = value["config_name"].As<std::string>();
    config.sharding_strategy =
        storages::redis::ToShardingStrategy(value["sharding_strategy"].As<std::string>("KeyShardTaximeterCrc32"));
    config.allow_reads_from_master = value["allow_reads_from_master"].As<bool>(false);
    return config;
}

struct SubscribeRedisGroup {
    std::string db;
    std::string config_name;
    storages::redis::ShardingStrategy sharding_strategy{storages::redis::ShardingStrategy::kKeyShardTaximeterCrc32};
    bool allow_reads_from_master{false};
};

SubscribeRedisGroup Parse(const yaml_config::YamlConfig& value, formats::parse::To<SubscribeRedisGroup>) {
    SubscribeRedisGroup config;
    config.db = value["db"].As<std::string>();
    config.config_name = value["config_name"].As<std::string>();
    config.sharding_strategy =
        storages::redis::ToShardingStrategy(value["sharding_strategy"].As<std::string>("KeyShardTaximeterCrc32"));
    config.allow_reads_from_master = value["allow_reads_from_master"].As<bool>(false);
    return config;
}

struct RedisPools {
    int sentinel_thread_pool_size;
    int redis_thread_pool_size;
};

RedisPools Parse(const yaml_config::YamlConfig& value, formats::parse::To<RedisPools>) {
    RedisPools pools{};
    pools.sentinel_thread_pool_size = value["sentinel_thread_pool_size"].As<int>();
    pools.redis_thread_pool_size = value["redis_thread_pool_size"].As<int>();
    return pools;
}

DynamicRedis::DynamicRedis(const ComponentConfig& config, const ComponentContext& component_context)
    : ComponentBase(config, component_context),
      testsuite_redis_control_(component_context.FindComponent<components::TestsuiteSupport>().GetRedisControl()),
      config_(component_context.FindComponent<DynamicConfig>().GetSource())
{
    const auto redis_pools = config["thread_pools"].As<RedisPools>();
    metrics_settings_.Assign(storages::redis::MetricsSettings());
    thread_pools_ = std::make_shared<
        storages::redis::impl::ThreadPools>(redis_pools.sentinel_thread_pool_size, redis_pools.redis_thread_pool_size);
    dynamic_redis_.Init(thread_pools_, testsuite_redis_control_);

    config_subscription_ = config_.UpdateAndListen(this, "dynamic-redis", &DynamicRedis::OnConfigUpdate);
    auto& statistics_storage = component_context.FindComponent<components::StatisticsStorage>().GetStorage();

    statistics_holder_ = statistics_storage.RegisterWriter(kStatisticsName, [this](utils::statistics::Writer& writer) {
        WriteStatistics(writer);
    });
}

DynamicRedis::~DynamicRedis() {
    statistics_holder_.Unregister();
    config_subscription_.Unsubscribe();
}

bool DynamicRedis::AddClient(const std::string& name, const storages::redis::DynamicSettings& dyn_settings) {
    return dynamic_redis_.AddClient(name, dyn_settings, config_);
}

bool DynamicRedis::RemoveClient(const std::string& name) { return dynamic_redis_.RemoveClient(name); }

utils::SharedRef<storages::redis::Client> DynamicRedis::GetDynamicClient(
    const std::string& name,
    storages::redis::RedisWaitConnected wait_connected
) const {
    return dynamic_redis_.GetDynamicClient(name, wait_connected);
}

std::unordered_set<std::string> DynamicRedis::ListDynamicClients() const { return dynamic_redis_.ListClients(); }

void DynamicRedis::OnConfigUpdate(const dynamic_config::Snapshot& cfg) { dynamic_redis_.OnConfigUpdate(cfg); }

void DynamicRedis::WriteStatistics(utils::statistics::Writer& writer) {
    auto settings = metrics_settings_.Read();
    dynamic_redis_.WriteStatistics(writer, *settings);
}

yaml_config::Schema DynamicRedis::GetStaticConfigSchema() {
    return yaml_config::MergeSchemasFromResource<ComponentBase>("src/storages/redis/dynamic_component.yaml");
}

}  // namespace components

USERVER_NAMESPACE_END
