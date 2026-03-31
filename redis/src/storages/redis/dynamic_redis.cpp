#include <userver/storages/redis/dynamic_redis.hpp>

#include <stdexcept>
#include <vector>

#include <fmt/ranges.h>

#include <userver/logging/log.hpp>
#include <userver/utils/statistics/writer.hpp>

#include <storages/redis/impl/sentinel.hpp>
#include <storages/redis/impl/thread_pools.hpp>
#include <userver/storages/redis/client.hpp>
#include <userver/storages/redis/redis_config.hpp>

#include "client_impl.hpp"

USERVER_NAMESPACE_BEGIN

namespace storages::redis {

void DynamicRedis::Init(
    std::shared_ptr<impl::ThreadPools> thread_pools,
    testsuite::RedisControl testsuite_redis_control
) {
    thread_pools_ = std::move(thread_pools);
    testsuite_redis_control_ = std::move(testsuite_redis_control);
}

DynamicRedis::~DynamicRedis() = default;

bool DynamicRedis::AddClient(
    const std::string& name,
    const DynamicSettings& dyn_settings,
    dynamic_config::Source& config
) {
    if (dyn_settings.sentinels.empty()) {
        const std::string error_msg = fmt::format("Failed to create sentinel for {}: sentinels list is empty", name);
        LOG_ERROR() << error_msg;
        throw std::invalid_argument(error_msg);
    }
    if (dyn_settings.shards.empty() &&
        (dyn_settings.sharding_strategy != ShardingStrategy::kRedisCluster &&
         dyn_settings.sharding_strategy != ShardingStrategy::kRedisStandalone))
    {
        const std::string error_msg = fmt::format("Failed to create sentinel for {}: shards list is empty", name);
        LOG_ERROR() << error_msg;
        throw std::invalid_argument(error_msg);
    }

    auto clients = dynamic_clients_.UniqueLock();
    if (clients->find(name) != clients->end()) {
        return false;
    }

    USERVER_NAMESPACE::secdist::RedisSettings settings;
    settings.database_index = dyn_settings.database_index;
    settings.password = dyn_settings.password;
    settings.secure_connection = dyn_settings.secure_connection;
    for (const auto& [host, port] : dyn_settings.sentinels) {
        settings.sentinels.emplace_back(host, port);
    }
    settings.sentinel_password = dyn_settings.sentinel_password;
    settings.shards = dyn_settings.shards;

    const auto& sharding_strategy = dyn_settings.sharding_strategy;

    CommandControl cc{};
    cc.allow_reads_from_master = dyn_settings.allow_reads_from_master;

    auto sentinel = impl::Sentinel::CreateSentinel(
        thread_pools_,
        settings,
        name,
        config,
        name,
        impl::KeyShardFactory{sharding_strategy},
        cc,
        testsuite_redis_control_
    );
    if (!sentinel) {
        const std::string error_msg = fmt::format("Failed to create sentinel for {}", name);
        LOG_ERROR() << error_msg;
        throw std::invalid_argument(error_msg);
    }

    const auto& client = std::make_shared<ClientImpl>(std::move(sentinel));
    clients->emplace(name, client);
    return true;
}

bool DynamicRedis::RemoveClient(const std::string& name) {
    auto clients = dynamic_clients_.UniqueLock();
    return clients->erase(name);
}

utils::SharedRef<Client> DynamicRedis::GetDynamicClient(
    const std::string& name,
    storages::redis::RedisWaitConnected wait_connected
) const {
    auto lock = dynamic_clients_.SharedLock();
    auto it = lock->find(name);
    if (it == lock->end()) {
        throw std::out_of_range(fmt::format("Failed to get client with name {}", name));
    }
    if (wait_connected.mode != WaitConnectedMode::kNoWait) {
        it->second->WaitConnectedOnce(wait_connected);
    }
    return utils::SharedRef<Client>(it->second);
}

std::unordered_set<std::string> DynamicRedis::ListClients() const {
    auto clients = dynamic_clients_.SharedLock();
    std::unordered_set<std::string> result;
    result.reserve(clients->size());
    for (const auto& [name, _] : *clients) {
        result.insert(name);
    }
    return result;
}

void DynamicRedis::WriteStatistics(utils::statistics::Writer& writer, const MetricsSettings& settings) const {
    auto dc = dynamic_clients_.SharedLock();
    for (const auto& [name, redis] : *dc) {
        auto stats = redis->GetNative().GetStatistics(settings);
        writer.ValueWithLabels(*stats, {"redis_database", name});
    }
}

void DynamicRedis::OnConfigUpdate(const dynamic_config::Snapshot& cfg) {
    const auto& redis_config = cfg[storages::redis::kConfig];
    auto cc = std::make_shared<storages::redis::CommandControl>(redis_config.default_command_control);

    auto clients = dynamic_clients_.SharedLock();
    for (auto& [name, client] : *clients) {
        if (!client) {
            continue;
        }

        auto& native = client->GetNative();
        native.SetConfigDefaultCommandControl(cc);
        native.SetCommandsBufferingSettings(redis_config.commands_buffering_settings);
        native.SetReplicationMonitoringSettings(redis_config.replication_monitoring_settings.GetOptional(name)
                                                    .value_or(storages::redis::ReplicationMonitoringSettings{}));
        native.SetRetryBudgetSettings(redis_config.retry_budget_settings.GetOptional(name)
                                          .value_or(utils::RetryBudgetSettings{}));
    }
}

}  // namespace storages::redis

USERVER_NAMESPACE_END
