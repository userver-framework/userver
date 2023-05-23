#include "subscribe_sentinel.hpp"

#include <userver/clients/dns/resolver.hpp>
#include <userver/logging/log.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/utils/assert.hpp>

#include "sentinel_impl.hpp"

USERVER_NAMESPACE_BEGIN

namespace redis {

namespace {

constexpr std::chrono::milliseconds kResolveTimeout{500};

ConnectionInfo::HostVector ResolveDns(const std::string& host,
                                      clients::dns::Resolver* resolver) {
  if (!resolver) return {};
  ConnectionInfo::HostVector result;
  try {
    auto addrs = resolver->Resolve(
        host, engine::Deadline::FromDuration(kResolveTimeout));
    std::transform(addrs.begin(), addrs.end(), std::back_inserter(result),
                   [](auto addr) { return addr.PrimaryAddressString(); });
  } catch (const std::exception& e) {
    LOG_WARNING() << "exception occurred during dns resolving for host " << host
                  << ": " << e;
  }
  return result;
}

}  // namespace

SubscribeSentinel::SubscribeSentinel(
    const std::shared_ptr<ThreadPools>& thread_pools,
    const std::vector<std::string>& shards,
    const std::vector<ConnectionInfo>& conns, std::string shard_group_name,
    dynamic_config::Source dynamic_config_source,
    const std::string& client_name, const Password& password,
    ConnectionSecurity connection_security, ReadyChangeCallback ready_callback,
    std::unique_ptr<KeyShard>&& key_shard, bool is_cluster_mode,
    CommandControl command_control,
    const testsuite::RedisControl& testsuite_redis_control)
    : Sentinel(thread_pools, shards, conns, std::move(shard_group_name),
               client_name, password, connection_security, ready_callback,
               dynamic_config_source, std::move(key_shard), command_control,
               testsuite_redis_control, ConnectionMode::kSubscriber),
      storage_(std::make_shared<SubscriptionStorage>(
          thread_pools, shards.size(), is_cluster_mode)),
      stopper_(std::make_shared<Stopper>()) {
  InitStorage();
  auto stopper = stopper_;
  signal_instances_changed.connect([this, stopper](size_t shard_idx) {
    std::lock_guard<std::mutex> lock(stopper->mutex);
    if (stopper->stopped) return;
    RebalanceSubscriptions(shard_idx);
  });
  signal_not_in_cluster_mode.connect(
      [this, stopper, thread_pools, shards_size = shards.size()]() {
        std::lock_guard<std::mutex> lock(stopper->mutex);
        if (stopper->stopped) return;
        storage_->SwitchToNonClusterMode();
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
    dynamic_config::Source dynamic_config_source,
    const std::string& client_name, bool is_cluster_mode,
    const testsuite::RedisControl& testsuite_redis_control,
    clients::dns::Resolver* dns_resolver) {
  auto ready_callback = [](size_t shard, const std::string& shard_name,
                           bool ready) {
    LOG_INFO() << "redis: ready_callback:"
               << "  shard = " << shard << "  shard_name = " << shard_name
               << "  ready = " << (ready ? "true" : "false");
  };
  // https://github.com/boostorg/signals2/issues/59
  // NOLINTNEXTLINE(clang-analyzer-cplusplus.NewDelete)
  return Create(thread_pools, settings, std::move(shard_group_name),
                dynamic_config_source, client_name, std::move(ready_callback),
                is_cluster_mode, testsuite_redis_control, dns_resolver);
}

std::shared_ptr<SubscribeSentinel> SubscribeSentinel::Create(
    const std::shared_ptr<ThreadPools>& thread_pools,
    const secdist::RedisSettings& settings, std::string shard_group_name,
    dynamic_config::Source dynamic_config_source,
    const std::string& client_name, ReadyChangeCallback ready_callback,
    bool is_cluster_mode,
    const testsuite::RedisControl& testsuite_redis_control,
    clients::dns::Resolver* dns_resolver) {
  const auto& password = settings.password;

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
    // SENTINEL MASTERS/SLAVES works without auth, sentinel has no AUTH command.
    // CLUSTER SLOTS works after auth only. Masters and slaves used instead of
    // sentinels in cluster mode.
    conns.emplace_back(sentinel.host, sentinel.port,
                       (is_cluster_mode ? password : Password("")), false,
                       settings.secure_connection,
                       ResolveDns(sentinel.host, dns_resolver));
  }
  redis::CommandControl command_control = redis::kDefaultCommandControl;
  LOG_DEBUG() << "redis command_control: timeout_single = "
              << command_control.timeout_single.count()
              << "ms; timeout_all = " << command_control.timeout_all.count()
              << "ms; max_retries = " << command_control.max_retries;

  auto subscribe_sentinel = std::make_shared<SubscribeSentinel>(
      thread_pools, shards, conns, std::move(shard_group_name),
      dynamic_config_source, client_name, password, settings.secure_connection,
      std::move(ready_callback),
      (is_cluster_mode ? nullptr : std::make_unique<KeyShardZero>()),
      is_cluster_mode, command_control, testsuite_redis_control);
  subscribe_sentinel->Start();
  return subscribe_sentinel;
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

PubsubClusterStatistics SubscribeSentinel::GetSubscriberStatistics(
    const PubsubMetricsSettings& settings) const {
  auto raw = storage_->GetStatistics();
  auto shards = GetMasterShards();
  UASSERT(raw.by_shard.size() == shards.size());

  PubsubClusterStatistics result(settings);
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

void SubscribeSentinel::InitStorage() {
  storage_->SetCommandControl(GetCommandControl({}));
  storage_->SetUnsubscribeCallback([this](size_t shard, CommandPtr cmd) {
    AsyncCommand(cmd, false, shard);
  });
  storage_->SetSubscribeCallback([this](size_t shard, CommandPtr cmd) {
    AsyncCommand(cmd, false, shard);
  });
}

}  // namespace redis

USERVER_NAMESPACE_END
