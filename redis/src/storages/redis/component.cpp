#include <userver/storages/redis/component.hpp>

#include <ranges>
#include <stdexcept>
#include <vector>

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

#include <storages/redis/impl/redis_group.hpp>
#include <storages/redis/impl/sentinel.hpp>
#include <storages/redis/impl/subscribe_sentinel.hpp>

#include "client_impl.hpp"
#include "redis_secdist.hpp"
#include "subscribe_client_impl.hpp"
#include "userver/storages/redis/base.hpp"
#include "userver/storages/redis/wait_connected_mode.hpp"

#ifndef ARCADIA_ROOT
#include "generated/src/storages/redis/component.yaml.hpp"  // Y_IGNORE
#endif

USERVER_NAMESPACE_BEGIN

namespace {

const auto kStatisticsName = "redis";
const auto kSubscribeStatisticsName = "redis-pubsub";

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

using storages::redis::impl::RedisGroup;
using storages::redis::impl::SubscribeRedisGroup;

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

Redis::Redis(const ComponentConfig& config, const ComponentContext& component_context)
    : ComponentBase(config, component_context),
      config_(component_context.FindComponent<DynamicConfig>().GetSource())
{
    const auto&
        testsuite_redis_control = component_context.FindComponent<components::TestsuiteSupport>().GetRedisControl();
    Connect(config, component_context, testsuite_redis_control);

    config_subscription_ = config_.UpdateAndListen(this, "redis", &Redis::OnConfigUpdate);

    auto& secdist = component_context.FindComponent<Secdist>();
    secdist_subscription_ = secdist.GetStorage().UpdateAndListen(this, "redis", &Redis::OnSecdistUpdate);

    utils::statistics::RegisterWriterScope(
        component_context,
        kStatisticsName,
        [this](utils::statistics::Writer& writer) { WriteStatistics(writer); }
    );

    utils::statistics::RegisterWriterScope(
        component_context,
        kSubscribeStatisticsName,
        [this](utils::statistics::Writer& writer) { WriteStatisticsPubsub(writer); }
    );
}

std::shared_ptr<storages::redis::Client> Redis::GetClient(
    const std::string& name,
    storages::redis::RedisWaitConnected wait_connected
) const {
    auto it = clients_.find(name);
    if (it == clients_.end()) {
        throw std::runtime_error(fmt::format(
            "{} redis client not found. Available clients: [{}]",
            name,
            fmt::join(clients_ | std::views::keys, ", ")
        ));
    }
    it->second->WaitConnectedOnce(wait_connected);
    return it->second;
}

std::shared_ptr<storages::redis::impl::Sentinel> Redis::Client(const std::string& name) const {
    auto it = sentinels_.find(name);
    if (it == sentinels_.end()) {
        throw std::runtime_error(fmt::format(
            "{} redis client not found. Available clients: [{}]",
            name,
            fmt::join(clients_ | std::views::keys, ", ")
        ));
    }
    return it->second;
}

std::shared_ptr<storages::redis::SubscribeClient> Redis::GetSubscribeClient(
    const std::string& name,
    storages::redis::RedisWaitConnected wait_connected
) const {
    auto it = subscribe_clients_.find(name);
    if (it == subscribe_clients_.end()) {
        throw std::runtime_error(fmt::format(
            "{} redis subscribe-client not found. Available subscribe-clients: "
            "[{}]",
            name,
            fmt::join(subscribe_clients_ | std::views::keys, ", ")
        ));
    }
    it->second->WaitConnectedOnce(wait_connected);
    return std::static_pointer_cast<storages::redis::SubscribeClient>(it->second);
}

void Redis::Connect(
    const ComponentConfig& config,
    const ComponentContext& component_context,
    const testsuite::RedisControl& testsuite_redis_control
) {
    auto& secdist_component = component_context.FindComponent<Secdist>();

    auto config_source = component_context.FindComponent<DynamicConfig>().GetSource();

    metrics_settings_.Assign(storages::redis::MetricsSettings());
    const auto redis_pools = config["thread_pools"].As<RedisPools>();

    thread_pools_ = std::make_shared<
        storages::redis::impl::ThreadPools>(redis_pools.sentinel_thread_pool_size, redis_pools.redis_thread_pool_size);

    const auto redis_groups = config["groups"].As<std::vector<RedisGroup>>();
    for (const RedisGroup& redis_group : redis_groups) {
        auto settings = GetSecdistSettings(secdist_component, redis_group);

        auto sentinel = storages::redis::impl::Sentinel::CreateSentinel(
            thread_pools_,
            settings,
            redis_group.config_name,
            config_source,
            storages::redis::impl::MakeSentinelStaticConfig(redis_group),
            testsuite_redis_control
        );
        if (sentinel) {
            sentinels_.emplace(redis_group.db, sentinel);
            const auto& client = std::make_shared<storages::redis::ClientImpl>(sentinel);
            clients_.emplace(redis_group.db, client);
        } else {
            LOG_WARNING() << "skip redis client for " << redis_group.db;
        }
    }

    auto cfg = config_.GetSnapshot();
    const auto& redis_config = cfg[storages::redis::kConfig];
    for (auto& sentinel_it : sentinels_) {
        sentinel_it.second->WaitConnectedOnce(redis_config.redis_wait_connected);
    }

    auto subscribe_redis_groups = config["subscribe_groups"].As<std::vector<SubscribeRedisGroup>>();

    for (const auto& redis_group : subscribe_redis_groups) {
        auto settings = GetSecdistSettings(secdist_component, redis_group);

        auto sentinel = storages::redis::impl::SubscribeSentinel::Create(
            thread_pools_,
            settings,
            redis_group.config_name,
            config_source,
            storages::redis::impl::MakeSubscribeSentinelStaticConfig(redis_group),
            testsuite_redis_control
        );
        if (sentinel) {
            subscribe_clients_
                .emplace(redis_group.db, std::make_shared<storages::redis::SubscribeClientImpl>(std::move(sentinel)));
        } else {
            LOG_WARNING() << "skip subscribe-redis client for " << redis_group.db;
        }
    }

    auto redis_wait_connected_subscribe = redis_config.redis_wait_connected;
    if (redis_wait_connected_subscribe.mode != storages::redis::WaitConnectedMode::kNoWait) {
        redis_wait_connected_subscribe.mode = storages::redis::WaitConnectedMode::kMasterOrSlave;
    }
    for (auto& subscribe_client_it : subscribe_clients_) {
        subscribe_client_it.second->WaitConnectedOnce(redis_wait_connected_subscribe);
    }
}

Redis::~Redis() { config_subscription_.Unsubscribe(); }

void Redis::WriteStatistics(utils::statistics::Writer& writer) {
    auto settings = metrics_settings_.Read();
    for (const auto& [name, redis] : sentinels_) {
        writer.ValueWithLabels(*redis->GetStatistics(*settings), {"redis_database", name});
    }
    auto threads_writer = writer["ev_threads"]["cpu_load_percent"];
    threads_writer.ValueWithLabels(*thread_pools_->GetRedisThreadPool(), {});
    threads_writer.ValueWithLabels(thread_pools_->GetSentinelThreadPool(), {});
}

void Redis::WriteStatisticsPubsub(utils::statistics::Writer& writer) {
    auto settings = pubsub_metrics_settings_.Read();
    for (const auto& [name, redis] : subscribe_clients_) {
        writer.ValueWithLabels(redis->GetNative().GetSubscriberStatistics(*settings), {"redis_database", name});
    }
}

void Redis::OnConfigUpdate(const dynamic_config::Snapshot& cfg) {
    LOG_INFO() << "update default command control";
    const auto& redis_config = cfg[storages::redis::kConfig];

    auto cc = std::make_shared<storages::redis::CommandControl>(redis_config.default_command_control);
    for (auto& it : sentinels_) {
        const auto& name = it.first;
        auto& client = it.second;
        client->SetConfigDefaultCommandControl(cc);
        client->SetCommandsBufferingSettings(redis_config.commands_buffering_settings);
        client->SetReplicationMonitoringSettings(redis_config.replication_monitoring_settings.GetOptional(name)
                                                     .value_or(storages::redis::ReplicationMonitoringSettings{}));
        client->SetRetryBudgetSettings(redis_config.retry_budget_settings.GetOptional(name)
                                           .value_or(utils::RetryBudgetSettings{}));
    }

    auto subscriber_cc = std::make_shared<
        storages::redis::CommandControl>(redis_config.subscriber_default_command_control);
    for (auto& it : subscribe_clients_) {
        auto& subscribe_client = it.second->GetNative();
        subscribe_client.SetConfigDefaultCommandControl(subscriber_cc);
        subscribe_client.SetRebalanceMinInterval(redis_config.subscriptions_rebalance_min_interval);
    }

    auto metrics_settings = metrics_settings_.Read();
    if (metrics_settings->dynamic_settings != redis_config.metrics_settings) {
        metrics_settings_.Assign(storages::redis::MetricsSettings(redis_config.metrics_settings));
    }

    auto pubsub_metrics_settings = pubsub_metrics_settings_.Read();
    if (*pubsub_metrics_settings != redis_config.pubsub_metrics_settings) {
        pubsub_metrics_settings_.Assign(redis_config.pubsub_metrics_settings);
    }
}

void Redis::OnSecdistUpdate(const storages::secdist::SecdistConfig& cfg) {
    for (auto& [db, sentinel] : sentinels_) {
        const auto& config_name = sentinel->ShardGroupName();
        const auto& settings = cfg.Get<storages::secdist::RedisMapSettings>().GetSettings(config_name);

        // TODO: move ConnectionInfo creation to Sentinel (must me same as in SubscribeSentinel::Create)
        std::vector<storages::redis::ConnectionInfo> cii;
        for (const auto& host_port : settings.sentinels) {
            const storages::redis::ConnectionInfo
                ci(host_port.host, host_port.port, storages::redis::Credentials{settings.username, settings.password});
            cii.push_back(ci);
        }

        sentinel->SetConnectionInfo(cii);
        sentinel->UpdateCredentials(storages::redis::Credentials{settings.username, settings.password});
    }
}

yaml_config::Schema Redis::GetStaticConfigSchema() {
    return yaml_config::MergeSchemasFromResource<ComponentBase>("src/storages/redis/component.yaml");
}

}  // namespace components

USERVER_NAMESPACE_END
