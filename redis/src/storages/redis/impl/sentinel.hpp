#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include <boost/signals2.hpp>

#include <userver/dynamic_config/source.hpp>
#include <userver/utils/retry_budget.hpp>
#include <userver/utils/swappingsmart.hpp>

#include <storages/redis/impl/request.hpp>
#include <storages/redis/impl/thread_pools.hpp>
#include <userver/storages/redis/base.hpp>
#include <userver/storages/redis/command_options.hpp>
#include <userver/storages/redis/fwd.hpp>
#include <userver/storages/redis/wait_connected_mode.hpp>

#include <storages/redis/impl/keyshard.hpp>
#include <storages/redis/impl/redis_stats.hpp>
#include <storages/redis/impl/secdist_redis.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::ev {
class ThreadControl;
}  // namespace engine::ev

namespace storages::redis::impl {

// We need only one thread for sentinels different from redis threads
const int kDefaultSentinelThreadPoolSize = 1;

// It works fine with 8 threads in driver_authorizer
const int kDefaultRedisThreadPoolSize = 8;

const auto kSentinelGetHostsCheckInterval = std::chrono::seconds(3);
const auto kProcessWaitingCommandsInterval = std::chrono::seconds(3);
const auto kCheckRedisConnectedInterval = std::chrono::seconds(3);

// Forward declarations
class SentinelImplBase;
class SentinelImpl;
class Shard;

class Sentinel {
public:
    /// Sentinel sends received message to callback and callback should
    /// notify it about the outcome. This is internal mechanism for
    /// communicating between our sentinel and our SubscriptionTokenImpl
    enum class Outcome : uint32_t {
        // everything is ok. Basically, means that message was pushed to the
        // SubscriptionQueue. Doesn't mean that actual user read it or processed
        // it or anything like that.
        kOk,
        // We discarded message because SubscriptionQueue was overflowing.
        kOverflowDiscarded,
    };

    Sentinel(
        const std::shared_ptr<ThreadPools>& thread_pools,
        const std::vector<std::string>& shards,
        const std::vector<ConnectionInfo>& conns,
        std::string shard_group_name,
        const std::string& client_name,
        const Password& password,
        ConnectionSecurity connection_security,
        dynamic_config::Source dynamic_config_source,
        KeyShardFactory key_shard_factory,
        CommandControl command_control,
        const testsuite::RedisControl& testsuite_redis_control,
        std::size_t database_index
    );
    virtual ~Sentinel();

    void Start();

    // Wait until connections to all servers are up
    void WaitConnectedDebug(bool allow_empty_slaves = false);

    // Wait until connections to all shards are up first time.
    // mode == kNoWait: do not wait.
    // mode == kMaster: for each shard need a connection to its master.
    // mode == kSlave: for each shard need a connection to at least one of its
    // slaves.
    // mode == kMasterOrSlave: for each shard need a connection to its master or
    // at least one of its slaves.
    // mode == kMasterAndSlave: for each shard need a connection to its master and
    // at least one of its slaves.
    void WaitConnectedOnce(RedisWaitConnected wait_connected);

    void ForceUpdateHosts();

    static std::shared_ptr<Sentinel> CreateSentinel(
        const std::shared_ptr<ThreadPools>& thread_pools,
        const USERVER_NAMESPACE::secdist::RedisSettings& settings,
        std::string shard_group_name,
        dynamic_config::Source dynamic_config_source,
        const std::string& client_name,
        KeyShardFactory key_shard_factory,
        const CommandControl& command_control = {},
        const testsuite::RedisControl& testsuite_redis_control = {}
    );

    void AsyncCommand(CommandPtr command, bool master = true, size_t shard = 0);
    void AsyncCommand(CommandPtr command, const std::string& key, bool master = true);

    // return a new temporary key with the same shard index
    static std::string CreateTmpKey(const std::string& key, std::string prefix = "tmp:");

    size_t ShardByKey(const std::string& key) const;
    size_t ShardsCount() const;
    bool IsInClusterMode() const noexcept { return is_in_cluster_mode_; }
    void CheckShardIdx(size_t shard_idx) const;
    static void CheckShardIdx(size_t shard_idx, size_t shard_count);

    // Returns a non-empty key of the minimum length consisting of lowercase
    // letters for a given shard.
    const std::string& GetAnyKeyForShard(size_t shard_idx) const;

    SentinelStatistics GetStatistics(const MetricsSettings& settings) const;

    void SetCommandsBufferingSettings(CommandsBufferingSettings commands_buffering_settings);
    void SetReplicationMonitoringSettings(const ReplicationMonitoringSettings& replication_monitoring_settings);
    void SetRetryBudgetSettings(const utils::RetryBudgetSettings& settings);

    Request MakeRequest(
        CmdArgs&& args,
        const std::string& key,
        bool master = true,
        const CommandControl& command_control = {},
        size_t replies_to_skip = 0
    ) {
        return {*this, std::forward<CmdArgs>(args), key, master, command_control, replies_to_skip};
    }

    Request MakeRequest(
        CmdArgs&& args,
        size_t shard,
        bool master = true,
        const CommandControl& command_control = {},
        size_t replies_to_skip = 0
    ) {
        return {*this, std::forward<CmdArgs>(args), shard, master, command_control, replies_to_skip};
    }

    CommandControl GetCommandControl(const CommandControl& cc) const;
    PublishSettings GetPublishSettings() const;

    virtual void SetConfigDefaultCommandControl(const std::shared_ptr<CommandControl>& cc);

    void SetConnectionInfo(std::vector<ConnectionInfo> info_array);
    const std::string& ShardGroupName() const;

    void UpdatePassword(const Password& password);

    using UserMessageCallback = std::function<Outcome(const std::string& channel, const std::string& message)>;
    using UserPmessageCallback =
        std::function<Outcome(const std::string& pattern, const std::string& channel, const std::string& message)>;

    using MessageCallback =
        std::function<void(ServerId server_id, const std::string& channel, const std::string& message)>;
    using PmessageCallback = std::function<
        void(ServerId server_id, const std::string& pattern, const std::string& channel, const std::string& message)>;
    using SubscribeCallback = std::function<void(ServerId, const std::string& channel, size_t count)>;
    using UnsubscribeCallback = std::function<void(ServerId, const std::string& channel, size_t count)>;

    virtual void NotifyInstancesChanged(size_t /*shard*/) {}
    virtual void NotifyTopologyChanged(size_t /*shards_count*/) {}

protected:
    void Stop() noexcept;

    std::unordered_map<ServerId, size_t, ServerIdHasher>
    GetAvailableServersWeighted(size_t shard_idx, bool with_master, const CommandControl& cc = {}) const;

public:
    static void OnSsubscribeReply(
        const MessageCallback& message_callback,
        const SubscribeCallback& subscribe_callback,
        const UnsubscribeCallback& unsubscribe_callback,
        ReplyPtr reply
    );

    static void OnSubscribeReply(
        const MessageCallback& message_callback,
        const SubscribeCallback& subscribe_callback,
        const UnsubscribeCallback& unsubscribe_callback,
        ReplyPtr reply
    );

    static void OnPsubscribeReply(
        const PmessageCallback& pmessage_callback,
        const SubscribeCallback& subscribe_callback,
        const UnsubscribeCallback& unsubscribe_callback,
        ReplyPtr reply
    );

private:
    friend class Transaction;

    std::unique_ptr<SentinelImplBase> impl_;
    const std::string shard_group_name_;
    std::shared_ptr<ThreadPools> thread_pools_;
    std::unique_ptr<engine::ev::ThreadControl> sentinel_thread_control_;
    CommandControl secdist_default_command_control_;
    utils::SwappingSmart<CommandControl> config_default_command_control_;
    testsuite::RedisControl testsuite_redis_control_;
    const bool is_in_cluster_mode_;
};

}  // namespace storages::redis::impl

USERVER_NAMESPACE_END
