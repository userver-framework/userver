#pragma once

#include <memory>
#include <mutex>
#include <vector>

#include <userver/testsuite/testsuite_support.hpp>

#include <storages/redis/impl/sentinel.hpp>
#include <storages/redis/impl/subscription_storage.hpp>

USERVER_NAMESPACE_BEGIN

namespace redis {

class SubscribeSentinel : protected Sentinel {
 public:
  SubscribeSentinel(
      const std::shared_ptr<ThreadPools>& thread_pools,
      const std::vector<std::string>& shards,
      const std::vector<ConnectionInfo>& conns, std::string shard_group_name,
      const std::string& client_name, const Password& password,
      ConnectionSecurity connection_security,
      ReadyChangeCallback ready_callback,
      std::unique_ptr<KeyShard>&& key_shard = nullptr,
      bool is_cluster_mode = false,
      CommandControl command_control = kDefaultCommandControl,
      const testsuite::RedisControl& testsuite_redis_control = {});
  ~SubscribeSentinel() override;

  static std::shared_ptr<SubscribeSentinel> Create(
      const std::shared_ptr<ThreadPools>& thread_pools,
      const secdist::RedisSettings& settings, std::string shard_group_name,
      const std::string& client_name, bool is_cluster_mode,
      const testsuite::RedisControl& testsuite_redis_control,
      clients::dns::Resolver* dns_resolver = nullptr);
  static std::shared_ptr<SubscribeSentinel> Create(
      const std::shared_ptr<ThreadPools>& thread_pools,
      const secdist::RedisSettings& settings, std::string shard_group_name,
      const std::string& client_name, ReadyChangeCallback ready_callback,
      bool is_cluster_mode,
      const testsuite::RedisControl& testsuite_redis_control,
      clients::dns::Resolver* dns_resolver = nullptr);

  SubscriptionToken Subscribe(
      const std::string& channel,
      const Sentinel::UserMessageCallback& message_callback,
      CommandControl control = CommandControl());
  SubscriptionToken Psubscribe(
      const std::string& pattern,
      const Sentinel::UserPmessageCallback& message_callback,
      CommandControl control = CommandControl());

  PubsubClusterStatistics GetSubscriberStatistics() const;

  void RebalanceSubscriptions(size_t shard_idx);

  void SetConfigDefaultCommandControl(
      const std::shared_ptr<CommandControl>& cc) override;

  void SetRebalanceMinInterval(std::chrono::milliseconds interval);

  using Sentinel::Restart;
  using Sentinel::SetConfigDefaultCommandControl;
  using Sentinel::WaitConnectedDebug;
  using Sentinel::WaitConnectedOnce;

 private:
  struct Stopper {
    std::mutex mutex;
    bool stopped{false};
  };

  void InitStorage();

  std::shared_ptr<SubscriptionStorage> storage_;
  std::shared_ptr<Stopper> stopper_;
};

}  // namespace redis

USERVER_NAMESPACE_END
