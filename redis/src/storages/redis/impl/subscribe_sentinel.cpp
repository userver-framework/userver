#include "subscribe_sentinel.hpp"

#include <logging/log.hpp>
#include <testsuite/testsuite_support.hpp>
#include <utils/assert.hpp>

#include "sentinel_impl.hpp"
#include "weak_bind.hpp"

namespace redis {

SubscribeSentinel::SubscribeSentinel(
    const std::shared_ptr<ThreadPools>& thread_pools,
    const std::vector<std::string>& shards,
    const std::vector<ConnectionInfo>& conns, std::string shard_group_name,
    const std::string& client_name, const std::string& password,
    ReadyChangeCallback ready_callback, std::unique_ptr<KeyShard>&& key_shard,
    bool is_cluster_mode, CommandControl command_control,
    const testsuite::RedisControl& testsuite_redis_control, bool track_masters,
    bool track_slaves)
    : Sentinel(thread_pools, shards, conns, std::move(shard_group_name),
               client_name, password, ready_callback, std::move(key_shard),
               command_control, testsuite_redis_control, track_masters,
               track_slaves),
      storage_(std::make_shared<SubscriptionStorage>(
          thread_pools, shards.size(), is_cluster_mode)),
      stopper_(std::make_shared<Stopper>()) {
  storage_->SetCommandControl(GetCommandControl({}));
  storage_->SetUnsubscribeCallback([this](size_t shard, CommandPtr cmd) {
    AsyncCommand(cmd, false, shard);
  });
  storage_->SetSubscribeCallback([this](size_t shard, CommandPtr cmd) {
    AsyncCommand(cmd, false, shard);
  });
  auto stopper = stopper_;
  signal_instances_changed.connect(
      [this, stopper](size_t shard_idx, bool /*master*/) {
        std::lock_guard<std::mutex> lock(stopper->mutex);
        if (stopper->stopped) return;
        RebalanceSubscriptions(shard_idx);
      });
}

SubscribeSentinel::~SubscribeSentinel() {
  {
    std::lock_guard<std::mutex> lock(stopper_->mutex);
    stopper_->stopped = true;
  }
  storage_->Stop();
}

std::shared_ptr<SubscribeSentinel> SubscribeSentinel::Create(
    const std::shared_ptr<ThreadPools>& thread_pools,
    const secdist::RedisSettings& settings, std::string shard_group_name,
    const std::string& client_name, bool is_cluster_mode,
    const testsuite::RedisControl& testsuite_redis_control) {
  auto ready_callback = [](size_t shard, const std::string& shard_name,
                           bool master, bool ready) {
    LOG_INFO() << "redis: ready_callback:"
               << "  shard = " << shard << "  shard_name = " << shard_name
               << "  master = " << (master ? "true" : "false")
               << "  ready = " << (ready ? "true" : "false");
  };
  return Create(thread_pools, settings, std::move(shard_group_name),
                client_name, std::move(ready_callback), is_cluster_mode,
                testsuite_redis_control);
}

std::shared_ptr<SubscribeSentinel> SubscribeSentinel::Create(
    const std::shared_ptr<ThreadPools>& thread_pools,
    const secdist::RedisSettings& settings, std::string shard_group_name,
    const std::string& client_name, ReadyChangeCallback ready_callback,
    bool is_cluster_mode,
    const testsuite::RedisControl& testsuite_redis_control) {
  const std::string& password = settings.password;

  const std::vector<std::string>& shards = settings.shards;
  LOG_DEBUG() << "shards.size() = " << shards.size();
  for (const std::string& shard : shards)
    LOG_DEBUG() << "shard:  name = " << shard;

  std::vector<ConnectionInfo> conns;
  conns.reserve(settings.sentinels.size());
  LOG_DEBUG() << "sentinels.size() = " << settings.sentinels.size();
  for (const auto& sentinel : settings.sentinels) {
    LOG_DEBUG() << "sentinel:  host = " << sentinel.host
                << "  port = " << sentinel.port;
    conns.emplace_back(sentinel.host, sentinel.port, "");
  }
  redis::CommandControl command_control = redis::command_control_init;
  LOG_DEBUG() << "redis command_control: timeout_single = "
              << command_control.timeout_single.count()
              << "ms; timeout_all = " << command_control.timeout_all.count()
              << "ms; max_retries = " << command_control.max_retries;

  return std::make_shared<SubscribeSentinel>(
      thread_pools, shards, conns, std::move(shard_group_name), client_name,
      password, std::move(ready_callback),
      (is_cluster_mode ? nullptr : std::make_unique<KeyShardZero>()),
      is_cluster_mode, command_control, testsuite_redis_control, true, true);
}

SubscriptionToken SubscribeSentinel::Subscribe(
    const std::string& channel,
    const Sentinel::UserMessageCallback& message_callback,
    CommandControl control) {
  auto token = storage_->Subscribe(channel, message_callback,
                                   GetCommandControl(control));
  return token;
}

SubscriptionToken SubscribeSentinel::Psubscribe(
    const std::string& pattern,
    const Sentinel::UserPmessageCallback& message_callback,
    CommandControl control) {
  auto token = storage_->Psubscribe(pattern, message_callback,
                                    GetCommandControl(control));
  return token;
}

PubsubClusterStatistics SubscribeSentinel::GetSubscriberStatistics() const {
  auto raw = storage_->GetStatistics();
  auto shards = GetMasterShards();
  UASSERT(raw.by_shard.size() == shards.size());

  PubsubClusterStatistics result;
  for (size_t i = 0; i < shards.size(); i++)
    result.by_shard.emplace(shards[i]->ShardName(), std::move(raw.by_shard[i]));
  return result;
}

void SubscribeSentinel::RebalanceSubscriptions(size_t shard_idx) {
  auto server_weights = GetAvailableServersWeighted(shard_idx, false);
  storage_->RequestRebalance(shard_idx, std::move(server_weights));
}

void SubscribeSentinel::SetConfigDefaultCommandControl(
    const std::shared_ptr<CommandControl>& cc) {
  Sentinel::SetConfigDefaultCommandControl(cc);
  storage_->SetCommandControl(GetCommandControl(*cc));
}

void SubscribeSentinel::SetRebalanceMinInterval(
    std::chrono::milliseconds interval) {
  storage_->SetRebalanceMinInterval(interval);
}

}  // namespace redis
