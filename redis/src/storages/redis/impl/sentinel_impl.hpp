#pragma once
#include <chrono>
#include <string>
#include <unordered_map>
#include <vector>

#include <boost/signals2/signal.hpp>

#include <engine/ev/thread_control.hpp>
#include <engine/ev/thread_pool.hpp>
#include <userver/concurrent/variable.hpp>
#include <userver/dynamic_config/source.hpp>
#include <userver/engine/deadline.hpp>
#include <userver/engine/impl/condition_variable_any.hpp>
#include <userver/utils/retry_budget.hpp>
#include <userver/utils/swappingsmart.hpp>

#include <storages/redis/impl/keyshard.hpp>
#include <storages/redis/impl/redis_stats.hpp>
#include <userver/storages/redis/client.hpp>
#include <userver/storages/redis/fwd.hpp>
#include <userver/storages/redis/wait_connected_mode.hpp>

#include "shard.hpp"

USERVER_NAMESPACE_BEGIN

namespace engine::ev {
class PeriodicWatcher;
}

namespace storages::redis::impl {
class TopologyHolderBase;

class SentinelImpl {
public:
    using ReadyChangeCallback = std::function<void(size_t shard, const std::string& shard_name, bool ready)>;

    static constexpr std::size_t kUnknownShard = std::numeric_limits<std::size_t>::max();
    static constexpr std::size_t kDefaultPrevInstanceIdx = std::numeric_limits<std::size_t>::max();

    struct SentinelCommand {
        CommandPtr command;
        bool master = true;
        size_t shard = kUnknownShard;
        std::chrono::steady_clock::time_point start;

        SentinelCommand() = default;
        SentinelCommand(CommandPtr command, bool master, size_t shard, std::chrono::steady_clock::time_point start)
            : command(command),
              master(master),
              shard(shard),
              start(start)
        {}
    };

    SentinelImpl(
        const engine::ev::ThreadControl& sentinel_thread_control,
        const std::shared_ptr<engine::ev::ThreadPool>& redis_thread_pool,
        Sentinel& sentinel,
        const std::vector<std::string>& shards,
        const std::vector<ConnectionInfo>& conns,
        std::string shard_group_name,
        const std::string& client_name,
        const Password& password,
        ConnectionSecurity connection_security,
        KeyShardFactory&& key_shard_factory,
        dynamic_config::Source dynamic_config_source,
        std::size_t database_index
    );
    ~SentinelImpl();

    std::unordered_map<ServerId, size_t, ServerIdHasher> GetAvailableServersWeighted(
        size_t shard_idx,
        bool with_master,
        const CommandControl& cc /*= {}*/
    ) const;

    void WaitConnectedDebug(bool allow_empty_slaves);

    void WaitConnectedOnce(RedisWaitConnected wait_connected);

    void ForceUpdateHosts();

    void AsyncCommand(const SentinelCommand& scommand, size_t prev_instance_idx /*= -1*/);

    size_t ShardByKey(const std::string& key) const;

    size_t ShardsCount() const;

    std::unique_ptr<SentinelStatistics> GetStatistics(const MetricsSettings& settings) const;

    void Start();
    void Stop();

    void SetCommandsBufferingSettings(CommandsBufferingSettings commands_buffering_settings);
    void SetReplicationMonitoringSettings(const ReplicationMonitoringSettings& replication_monitoring_settings);
    void SetRetryBudgetSettings(const utils::RetryBudgetSettings& settings);
    PublishSettings GetPublishSettings();

    static size_t GetClusterSlotsCalledCounter();

    void SetConnectionInfo(const std::vector<ConnectionInfoInt>& info_array);
    void UpdatePassword(const Password& password);

private:
    void Init();  // used from constructor

    void AsyncCommandFailed(const SentinelCommand& scommand);
    void EnqueueCommand(const SentinelCommand& command);

    Sentinel& sentinel_obj_;
    engine::ev::ThreadControl ev_thread_;
    std::atomic_bool delete_started_{false};

    std::unique_ptr<engine::ev::PeriodicWatcher> process_waiting_commands_timer_;
    void ProcessWaitingCommands();
    void ProcessWaitingCommandsOnStop();

    std::unique_ptr<TopologyHolderBase> topology_holder_;
    const KeyShardFactory key_shard_factory_;
    const std::unique_ptr<KeyShard> key_shard_;
    std::atomic<int> publish_shard_{0};

    const std::string shard_group_name_;
    logging::LogExtra log_extra_;
    const std::vector<ConnectionInfo> conns_;

    std::shared_ptr<engine::ev::ThreadPool> redis_thread_pool_;

    const std::string client_name_;

    std::vector<SentinelCommand> commands_;
    std::mutex command_mutex_;

    SentinelStatisticsInternal statistics_internal_;

    dynamic_config::Source dynamic_config_source_;
    const std::size_t database_index_{0};
};

}  // namespace storages::redis::impl

USERVER_NAMESPACE_END
