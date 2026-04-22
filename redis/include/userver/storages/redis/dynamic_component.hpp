#pragma once

/// @file userver/storages/redis/dynamic_component.hpp
/// @brief @copybrief components::DynamicRedis

#include <memory>
#include <string>

#include <userver/components/component_base.hpp>
#include <userver/components/component_fwd.hpp>
#include <userver/concurrent/variable.hpp>
#include <userver/dynamic_config/source.hpp>
#include <userver/engine/shared_mutex.hpp>
#include <userver/rcu/rcu.hpp>
#include <userver/storages/redis/base.hpp>
#include <userver/storages/redis/dynamic_redis.hpp>
#include <userver/storages/redis/fwd.hpp>
#include <userver/storages/redis/sharding_strategies.hpp>
#include <userver/storages/redis/wait_connected_mode.hpp>
#include <userver/storages/secdist/secdist.hpp>
#include <userver/testsuite/redis_control.hpp>
#include <userver/utils/statistics/entry.hpp>

USERVER_NAMESPACE_BEGIN

/// Components, clients and helpers for different databases and storages
namespace storages {}

/// Valkey and Redis client and helpers
namespace storages::redis {

class SubscribeClientImpl;

namespace impl {
class Sentinel;
class ThreadPools;
}  // namespace impl
}  // namespace storages::redis

namespace components {

/// @ingroup userver_components
///
/// @brief Valkey and Redis dynamic client component, that does not require secdist
///
/// Provides access to a valkey or redis cluster.
///
/// ## Dynamic options:
/// Same as for @ref components::Redis
///
/// ## Static options of components::Redis :
/// @include{doc} scripts/docs/en/components_schema/redis/src/storages/redis/dynamic_component.md
///
/// Options inherited from @ref components::ComponentBase :
/// @include{doc} scripts/docs/en/components_schema/core/src/components/impl/component_base.md
class DynamicRedis : public ComponentBase {
public:
    DynamicRedis(const ComponentConfig& config, const ComponentContext& component_context);

    ~DynamicRedis() override;

    /// @ingroup userver_component_names
    /// @brief The default name of components::DynamicRedis
    static constexpr std::string_view kName = "dynamic-redis";

    /// @brief Adds a new client with the specified name and settings.
    ///
    /// @param name the name of the client
    /// @param settings the dynamic settings for the client
    /// @return true if the client was added, false if a client with the same name already exists
    bool AddClient(const std::string& name, const storages::redis::DynamicSettings& settings);

    /// @brief Removes a client with the specified name.
    ///
    /// @param name the name of the client to remove
    /// @return true if the client was removed, false if no client with the specified name exists
    bool RemoveClient(const std::string& name);

    /// @brief Retrieves a dynamically added client by name.
    ///
    /// @param name the name of the client
    /// @param wait_connected wait mode for the client connection
    /// @return shared reference to the client or throws out_of_range exception if not found
    utils::SharedRef<storages::redis::Client> GetDynamicClient(
        const std::string& name,
        storages::redis::RedisWaitConnected wait_connected = {}
    ) const;

    /// @brief Lists the names of all dynamically added clients.
    ///
    /// @return a set of client names
    std::unordered_set<std::string> ListDynamicClients() const;

    static yaml_config::Schema GetStaticConfigSchema();

private:
    void OnConfigUpdate(const dynamic_config::Snapshot& cfg);

    void WriteStatistics(utils::statistics::Writer& writer);

    std::shared_ptr<storages::redis::impl::ThreadPools> thread_pools_;
    storages::redis::DynamicRedis dynamic_redis_;
    testsuite::RedisControl testsuite_redis_control_;
    dynamic_config::Source config_;
    concurrent::AsyncEventSubscriberScope config_subscription_;

    utils::statistics::Entry statistics_holder_;
    rcu::Variable<storages::redis::MetricsSettings> metrics_settings_;
};

template <>
inline constexpr bool kHasValidate<DynamicRedis> = true;

}  // namespace components

USERVER_NAMESPACE_END
