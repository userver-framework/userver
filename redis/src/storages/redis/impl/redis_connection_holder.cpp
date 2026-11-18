#include "redis_connection_holder.hpp"

#include <initializer_list>

#include <userver/logging/log.hpp>
#include <userver/rcu/rcu.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::redis::impl {

class RedisConnectionHolder::EmplaceEnabler {};

struct RedisConnectionHolder::ConnectionState {
    struct Readiness {
        bool was_ever_connected{false};
        std::chrono::steady_clock::time_point disconnected_time;
    };

    explicit ConnectionState(Statistics& statistics)
        : statistics(statistics)
    {}

    Statistics& statistics;
    rcu::Variable<std::shared_ptr<Redis>, rcu::ExclusiveRcuTraits> redis;
    rcu::Variable<Readiness, rcu::ExclusiveRcuTraits> readiness;
};

RedisConnectionHolder::RedisConnectionHolder(
    EmplaceEnabler,
    const engine::ev::ThreadControl& sentinel_thread_control,
    const std::shared_ptr<engine::ev::ThreadPool>& redis_thread_pool,
    const std::string& shard_group_name,
    const std::string& host,
    uint16_t port,
    Credentials credentials,
    std::size_t database_index,
    CommandsBufferingSettings buffering_settings,
    ReplicationMonitoringSettings replication_monitoring_settings,
    utils::RetryBudgetSettings retry_budget_settings,
    redis::RedisCreationSettings redis_creation_settings,
    Statistics& stats,
    Hysteresis::Config config,
    std::chrono::seconds max_disconnect_time,
    ConnectionKind kind
)
    : commands_buffering_settings_(std::move(buffering_settings)),
      replication_monitoring_settings_(std::move(replication_monitoring_settings)),
      retry_budget_settings_(std::move(retry_budget_settings)),
      ev_thread_(sentinel_thread_control),
      redis_thread_pool_(redis_thread_pool),
      shard_group_name_(shard_group_name),
      host_(host),
      port_(port),
      credentials_(std::move(credentials)),
      database_index_(database_index),
      command_connection_(kind == ConnectionKind::kCommand ? std::make_unique<ConnectionState>(stats) : nullptr),
      subscription_connection_(
          kind == ConnectionKind::kSubscription ? std::make_unique<ConnectionState>(stats) : nullptr
      ),
      connection_check_timer_(ev_thread_, [this] { EnsureConnected(); }, kCheckRedisConnectedInterval),
      redis_creation_settings_(redis_creation_settings),
      failed_hysteresis_(config),
      max_disconnect_time_(max_disconnect_time)
{}

RedisConnectionHolder::~RedisConnectionHolder() { connection_check_timer_.Stop(); }

std::shared_ptr<RedisConnectionHolder> RedisConnectionHolder::Create(
    const engine::ev::ThreadControl& sentinel_thread_control,
    const std::shared_ptr<engine::ev::ThreadPool>& redis_thread_pool,
    const std::string& shard_group_name,
    const std::string& host,
    uint16_t port,
    Credentials credentials,
    std::size_t database_index,
    CommandsBufferingSettings buffering_settings,
    ReplicationMonitoringSettings replication_monitoring_settings,
    utils::RetryBudgetSettings retry_budget_settings,
    Statistics& stats,
    redis::RedisCreationSettings redis_creation_settings,
    Hysteresis::Config config,
    std::chrono::seconds max_disconnect_time,
    ConnectionKind kind
) {
    auto holder = std::make_shared<RedisConnectionHolder>(
        EmplaceEnabler{},
        sentinel_thread_control,
        redis_thread_pool,
        shard_group_name,
        host,
        port,
        std::move(credentials),
        database_index,
        std::move(buffering_settings),
        std::move(replication_monitoring_settings),
        std::move(retry_budget_settings),
        std::move(redis_creation_settings),
        stats,
        config,
        max_disconnect_time,
        kind
    );

    // https://github.com/boostorg/signals2/issues/59
    // NOLINTNEXTLINE(clang-analyzer-cplusplus.NewDelete)
    holder->CreateConnection(kind, *holder->GetConnectionState(kind));
    holder->connection_check_timer_.Start();

    return holder;
}

std::shared_ptr<Redis> RedisConnectionHolder::GetConnection(ConnectionKind kind) const {
    const auto* connection = GetConnectionState(kind);
    return connection ? connection->redis.ReadCopy() : nullptr;
}

RedisConnectionHolder::ConnectionState* RedisConnectionHolder::GetConnectionState(ConnectionKind kind) const {
    return kind == ConnectionKind::kCommand ? command_connection_.get() : subscription_connection_.get();
}

void RedisConnectionHolder::EnableSubscriptions(Statistics& subscription_statistics) {
    UASSERT(ev_thread_.IsInEvThread());
    UASSERT(command_connection_);
    UASSERT(!subscription_connection_);
    auto connection = std::make_unique<ConnectionState>(subscription_statistics);
    CreateConnection(ConnectionKind::kSubscription, *connection);
    subscription_connection_ = std::move(connection);
}

void RedisConnectionHolder::EnsureConnected() {
    for (const auto kind : {ConnectionKind::kCommand, ConnectionKind::kSubscription}) {
        auto* connection = GetConnectionState(kind);
        if (!connection) {
            continue;
        }
        const auto redis = connection->redis.ReadCopy();
        if (redis && (redis->GetState() == Redis::State::kConnected || redis->GetState() == Redis::State::kInit)) {
            continue;
        }
        // https://github.com/boostorg/signals2/issues/59
        // NOLINTNEXTLINE(clang-analyzer-cplusplus.NewDelete)
        CreateConnection(kind, *connection);
    }
}

void RedisConnectionHolder::CreateConnection(ConnectionKind kind, ConnectionState& connection) {
    UASSERT(ev_thread_.IsInEvThread());
    auto instance = std::make_shared<
        Redis>(redis_thread_pool_, redis_creation_settings_, shard_group_name_, connection.statistics);
    UASSERT(weak_from_this().lock());
    instance->signal_state_change
        .connect([weak_holder{weak_from_this()}, weak_instance{std::weak_ptr{instance}}, kind](Redis::State state) {
            const auto holder = weak_holder.lock();
            if (!holder) {
                return;
            }

            holder->ev_thread_.RunInEvLoopAsync([weak_holder, weak_instance, kind, state]() noexcept {
                const auto holder = weak_holder.lock();
                const auto instance = weak_instance.lock();
                if (!holder || !instance) {
                    return;
                }
                auto* connection = holder->GetConnectionState(kind);
                if (!connection || connection->redis.ReadCopy() != instance) {
                    return;
                }
                try {
                    holder->OnStateChanged(*connection, state);
                } catch (const std::exception& ex) {
                    LOG_ERROR() << "Failed to process Redis state change: " << ex;
                }
            });
        });

    if (kind == ConnectionKind::kCommand) {
        if (commands_buffering_settings_.has_value()) {
            instance->SetCommandsBufferingSettings(*commands_buffering_settings_);
        }
        instance->SetReplicationMonitoringSettings(replication_monitoring_settings_);
    }
    instance->SetRetryBudgetSettings(retry_budget_settings_);

    instance->Connect({host_}, port_, credentials_, database_index_);
    connection.redis.Assign(std::move(instance));
}

void RedisConnectionHolder::OnStateChanged(ConnectionState& connection, Redis::State state) {
    UASSERT(ev_thread_.IsInEvThread());
    auto readiness = connection.readiness.ReadCopy();
    switch (state) {
        case RedisState::kConnected: {
            readiness.disconnected_time = std::chrono::steady_clock::time_point();
            readiness.was_ever_connected = true;
            break;
        }
        case RedisState::kInit:
        case RedisState::kInitError:
        case RedisState::kDisconnected:
        case RedisState::kDisconnecting:
        case RedisState::kDisconnectError: {
            if (readiness.disconnected_time == std::chrono::steady_clock::time_point()) {
                readiness.disconnected_time = std::chrono::steady_clock::now();
            }
            break;
        }
    }

    connection.readiness.Assign(std::move(readiness));
    signal_state_change(state);
}

void RedisConnectionHolder::SetReplicationMonitoringSettings(ReplicationMonitoringSettings settings) {
    UASSERT(ev_thread_.IsInEvThread());
    if (command_connection_) {
        replication_monitoring_settings_ = settings;
        command_connection_->redis.ReadCopy()->SetReplicationMonitoringSettings(std::move(settings));
    }
}

void RedisConnectionHolder::SetCommandsBufferingSettings(CommandsBufferingSettings settings) {
    UASSERT(ev_thread_.IsInEvThread());
    if (command_connection_) {
        commands_buffering_settings_ = settings;
        command_connection_->redis.ReadCopy()->SetCommandsBufferingSettings(std::move(settings));
    }
}

void RedisConnectionHolder::SetRetryBudgetSettings(utils::RetryBudgetSettings settings) {
    UASSERT(ev_thread_.IsInEvThread());
    retry_budget_settings_ = settings;
    for (const auto kind : {ConnectionKind::kCommand, ConnectionKind::kSubscription}) {
        if (auto* connection = GetConnectionState(kind)) {
            connection->redis.ReadCopy()->SetRetryBudgetSettings(settings);
        }
    }
}

void RedisConnectionHolder::UpdateCredentials(const Credentials& credentials) {
    UASSERT(ev_thread_.IsInEvThread());
    if (credentials_.username == credentials.username &&
        credentials_.password.GetUnderlying() == credentials.password.GetUnderlying())
    {
        return;
    }
    credentials_ = credentials;
    for (const auto kind : {ConnectionKind::kCommand, ConnectionKind::kSubscription}) {
        if (auto* connection = GetConnectionState(kind)) {
            CreateConnection(kind, *connection);
        }
    }
}

bool RedisConnectionHolder::IsReady(ConnectionKind kind) const noexcept {
    const auto* connection = GetConnectionState(kind);
    if (!connection) {
        return false;
    }
    if (failed_hysteresis_.IsFailed()) {
        return true;
    }

    const auto readiness = connection->readiness.Read();

    // workaround to allow WaitConnectedOnce to work
    if (!readiness->was_ever_connected && GetState(kind) != RedisState::kConnected) {
        return false;
    }

    const auto now = std::chrono::steady_clock::now();
    if (readiness->disconnected_time != std::chrono::steady_clock::time_point() &&
        (now - readiness->disconnected_time) > max_disconnect_time_)
    {
        return false;
    }

    return true;
}

Redis::State RedisConnectionHolder::GetState(ConnectionKind kind) const {
    const auto connection = GetConnection(kind);
    return connection ? connection->GetState() : RedisState::kDisconnected;
}

size_t RedisConnectionHolder::GetCommandsCounter(ConnectionKind kind) const {
    const auto connection = GetConnection(kind);
    return connection ? connection->GetStatistics().commands_count.load(std::memory_order_relaxed) : 0;
}

}  // namespace storages::redis::impl

USERVER_NAMESPACE_END
