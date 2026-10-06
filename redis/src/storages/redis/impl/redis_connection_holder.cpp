#include "redis_connection_holder.hpp"

#include <userver/logging/log.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::redis::impl {

class RedisConnectionHolder::EmplaceEnabler {};

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
    std::chrono::seconds max_disconnect_time
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
      statistics_(stats),
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
    std::chrono::seconds max_disconnect_time
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
        max_disconnect_time
    );

    // https://github.com/boostorg/signals2/issues/59
    // NOLINTNEXTLINE(clang-analyzer-cplusplus.NewDelete)
    holder->CreateConnection();
    holder->connection_check_timer_.Start();

    return holder;
}

std::shared_ptr<Redis> RedisConnectionHolder::Get() const { return redis_.ReadCopy(); }

void RedisConnectionHolder::EnsureConnected() {
    auto redis = redis_.ReadCopy();
    if (redis && (redis->GetState() == Redis::State::kConnected || redis->GetState() == Redis::State::kInit)) {
        return;
    }
    // https://github.com/boostorg/signals2/issues/59
    // NOLINTNEXTLINE(clang-analyzer-cplusplus.NewDelete)
    CreateConnection();
}

void RedisConnectionHolder::CreateConnection() {
    UASSERT(ev_thread_.IsInEvThread());
    auto instance = std::make_shared<
        Redis>(redis_thread_pool_, redis_creation_settings_, shard_group_name_, statistics_);
    UASSERT(weak_from_this().lock());
    instance->signal_state_change
        .connect([weak_holder{weak_from_this()}, weak_instance{std::weak_ptr{instance}}](Redis::State state) {
            const auto holder = weak_holder.lock();
            if (!holder) {
                return;
            }

            holder->ev_thread_.RunInEvLoopAsync([weak_holder, weak_instance, state]() noexcept {
                const auto holder = weak_holder.lock();
                const auto instance = weak_instance.lock();
                if (!holder || !instance || holder->redis_.ReadCopy() != instance) {
                    return;
                }
                try {
                    holder->OnStateChanged(state);
                } catch (const std::exception& ex) {
                    LOG_ERROR() << "Failed to process Redis state change: " << ex;
                }
            });
        });

    if (commands_buffering_settings_.has_value()) {
        instance->SetCommandsBufferingSettings(*commands_buffering_settings_);
    }
    instance->SetReplicationMonitoringSettings(replication_monitoring_settings_);
    instance->SetRetryBudgetSettings(retry_budget_settings_);

    instance->Connect({host_}, port_, credentials_, database_index_);
    redis_.Assign(std::move(instance));
}

void RedisConnectionHolder::OnStateChanged(Redis::State state) {
    UASSERT(ev_thread_.IsInEvThread());
    auto readiness = readiness_state_.ReadCopy();
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

    readiness_state_.Assign(std::move(readiness));
    signal_state_change(state);
}

void RedisConnectionHolder::SetReplicationMonitoringSettings(ReplicationMonitoringSettings settings) {
    UASSERT(ev_thread_.IsInEvThread());
    replication_monitoring_settings_ = settings;
    redis_.ReadCopy()->SetReplicationMonitoringSettings(std::move(settings));
}

void RedisConnectionHolder::SetCommandsBufferingSettings(CommandsBufferingSettings settings) {
    UASSERT(ev_thread_.IsInEvThread());
    commands_buffering_settings_ = settings;
    redis_.ReadCopy()->SetCommandsBufferingSettings(std::move(settings));
}

void RedisConnectionHolder::SetRetryBudgetSettings(utils::RetryBudgetSettings settings) {
    UASSERT(ev_thread_.IsInEvThread());
    retry_budget_settings_ = settings;
    redis_.ReadCopy()->SetRetryBudgetSettings(std::move(settings));
}

bool RedisConnectionHolder::IsReady() const noexcept {
    if (failed_hysteresis_.IsFailed()) {
        return true;
    }

    const auto readiness = readiness_state_.Read();

    // workaround to allow WaitConnectedOnce to work
    if (!readiness->was_ever_connected && GetState() != RedisState::kConnected) {
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

Redis::State RedisConnectionHolder::GetState() const {
    auto ptr = redis_.Read();
    return ptr->get()->GetState();
}

size_t RedisConnectionHolder::GetCommandsCounter() const {
    auto ptr = redis_.Read();
    return ptr->get()->GetStatistics().commands_count.load(std::memory_order_relaxed);
}

}  // namespace storages::redis::impl

USERVER_NAMESPACE_END
