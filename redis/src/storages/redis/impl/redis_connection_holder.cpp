#include "redis_connection_holder.hpp"

USERVER_NAMESPACE_BEGIN

namespace redis {

RedisConnectionHolder::RedisConnectionHolder(
    const engine::ev::ThreadControl& sentinel_thread_control,
    const std::shared_ptr<engine::ev::ThreadPool>& redis_thread_pool,
    const std::string& host, uint16_t port, Password password,
    CommandsBufferingSettings buffering_settings,
    ReplicationMonitoringSettings replication_monitoring_settings)
    : commands_buffering_settings_(std::move(buffering_settings)),
      replication_monitoring_settings_(
          std::move(replication_monitoring_settings)),
      ev_thread_(sentinel_thread_control),
      redis_thread_pool_(redis_thread_pool),
      host_(host),
      port_(port),
      password_(std::move(password)),
      connection_check_timer_(
          ev_thread_, [this] { EnsureConnected(); },
          kCheckRedisConnectedInterval) {
  // https://github.com/boostorg/signals2/issues/59
  // NOLINTNEXTLINE(clang-analyzer-cplusplus.NewDelete)
  CreateConnection();
  ev_thread_.RunInEvLoopAsync([this] { connection_check_timer_.Start(); });
}

RedisConnectionHolder::~RedisConnectionHolder() {
  ev_thread_.RunInEvLoopBlocking([this] { connection_check_timer_.Stop(); });
}

std::shared_ptr<Redis> RedisConnectionHolder::Get() const {
  return redis_.ReadCopy();
}

void RedisConnectionHolder::EnsureConnected() {
  auto redis = redis_.ReadCopy();
  if (redis && (redis->GetState() == Redis::State::kConnected ||
                redis->GetState() == Redis::State::kInit)) {
    return;
  }
  // https://github.com/boostorg/signals2/issues/59
  // NOLINTNEXTLINE(clang-analyzer-cplusplus.NewDelete)
  CreateConnection();
}

void RedisConnectionHolder::CreateConnection() {
  RedisCreationSettings settings;
  /// Here we allow read from replicas possibly stale data.
  /// This does not affect connections to masters
  settings.send_readonly = true;
  auto instance = std::make_shared<Redis>(redis_thread_pool_, settings);
  instance->signal_state_change.connect(
      [weak_ptr{weak_from_this()}](Redis::State state) {
        const auto ptr = weak_ptr.lock();
        if (!ptr) return;

        ptr->signal_state_change(state);
      });

  {
    auto settings_ptr = commands_buffering_settings_.Lock();
    if (settings_ptr->has_value()) {
      instance->SetCommandsBufferingSettings(settings_ptr->value());
    }
  }
  {
    auto settings_ptr = replication_monitoring_settings_.Lock();
    instance->SetReplicationMonitoringSettings(*settings_ptr);
  }

  instance->Connect({host_}, port_, password_);
  redis_.Assign(std::move(instance));
}

void RedisConnectionHolder::SetReplicationMonitoringSettings(
    ReplicationMonitoringSettings settings) {
  auto ptr = replication_monitoring_settings_.Lock();
  *ptr = settings;
  redis_.ReadCopy()->SetReplicationMonitoringSettings(std::move(settings));
}

void RedisConnectionHolder::SetCommandsBufferingSettings(
    CommandsBufferingSettings settings) {
  auto ptr = commands_buffering_settings_.Lock();
  *ptr = settings;
  redis_.ReadCopy()->SetCommandsBufferingSettings(std::move(settings));
}

Redis::State RedisConnectionHolder::GetState() const {
  auto ptr = redis_.Read();
  return ptr->get()->GetState();
}

}  // namespace redis

USERVER_NAMESPACE_END
