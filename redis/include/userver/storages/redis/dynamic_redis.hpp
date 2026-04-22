#pragma once

/// @file userver/storages/redis/dynamic_redis.hpp
/// @brief @copybrief storages::redis::DynamicRedis

#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include <userver/concurrent/variable.hpp>
#include <userver/dynamic_config/source.hpp>
#include <userver/engine/shared_mutex.hpp>
#include <userver/storages/redis/base.hpp>
#include <userver/storages/redis/fwd.hpp>
#include <userver/storages/redis/sharding_strategies.hpp>
#include <userver/storages/redis/wait_connected_mode.hpp>
#include <userver/testsuite/redis_control.hpp>
#include <userver/utils/not_null.hpp>
#include <userver/utils/statistics/entry.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::redis {

/// @brief Dynamic configuration settings for Redis clients
struct DynamicSettings final {
    struct HostPort {
        std::string host;
        int port;

        explicit HostPort(std::string host = {}, int port = 0) noexcept : host(std::move(host)), port(port) {}
    };

    std::vector<std::string> shards;
    std::vector<HostPort> sentinels;
    /// Password for nodes
    storages::redis::Password password{std::string()};
    /// Password for sentinels. Available since Redis 5.0.1. For early versions should be always empty.
    storages::redis::Password sentinel_password{std::string()};
    storages::redis::ConnectionSecurity secure_connection{storages::redis::ConnectionSecurity::kNone};
    std::size_t database_index{0};
    ShardingStrategy sharding_strategy;
    bool allow_reads_from_master = false;
};

class ClientImpl;

namespace impl {
class Sentinel;
class ThreadPools;
}  // namespace impl

/// @brief Manages dynamically created Redis clients
class DynamicRedis {
public:
    DynamicRedis() = default;

    void Init(std::shared_ptr<impl::ThreadPools> thread_pools, testsuite::RedisControl testsuite_redis_control);

    ~DynamicRedis();

    /// @brief Adds a new client with the specified name and settings.
    ///
    /// @param name the name of the client
    /// @param settings the dynamic settings for the client
    /// @return true if the client was added, false if a client with the same name already exists
    bool AddClient(const std::string& name, const DynamicSettings& settings, dynamic_config::Source& config);

    /// @brief Removes a client with the specified name.
    ///
    /// @param name the name of the client to remove
    /// @return true if the client was removed, false if no client with the specified name exists
    bool RemoveClient(const std::string& name);

    /// @brief Retrieves a dynamically added client by name.
    ///
    /// @param name the name of the client
    /// @param wait_connected wait mode for the client connection
    /// @return shared pointer to the client
    /// throws std::out_of_range exception if there is no client with requested name
    utils::SharedRef<Client> GetDynamicClient(
        const std::string& name,
        storages::redis::RedisWaitConnected wait_connected = {}
    ) const;

    /// @brief Lists the names of all dynamically added clients.
    ///
    /// @return a set of client names
    std::unordered_set<std::string> ListClients() const;

    /// @brief Writes statistics for all dynamic clients.
    ///
    /// @param writer statistics writer
    void WriteStatistics(utils::statistics::Writer& writer, const MetricsSettings& settings) const;

    void OnConfigUpdate(const dynamic_config::Snapshot& cfg);

private:
    std::shared_ptr<impl::ThreadPools> thread_pools_;
    testsuite::RedisControl testsuite_redis_control_;

    concurrent::Variable<std::unordered_map<std::string, std::shared_ptr<ClientImpl>>, engine::SharedMutex>
        dynamic_clients_;
};

}  // namespace storages::redis

USERVER_NAMESPACE_END
