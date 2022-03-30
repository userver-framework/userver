#pragma once

#include <memory>
#include <mutex>
#include <vector>

#include <userver/storages/redis/impl/sentinel.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include "subscription_storage.hpp"

USERVER_NAMESPACE_BEGIN

namespace redis {

class SubscribeSentinel : protected Sentinel {
 public:
  SubscribeSentinel(const std::shared_ptr<ThreadPools>& thread_pools,
                    const std::vector<std::string>& shards,
                    const std::vector<ConnectionInfo>& conns,
                    std::string shard_group_name,
                    const std::string& client_name, const Password& password,
                    ReadyChangeCallback ready_callback,
                    std::unique_ptr<KeyShard>&& key_shard = nullptr,
                    bool is_cluster_mode = false,
                    CommandControl command_control = command_control_init,
                    const testsuite::RedisControl& testsuite_redis_control = {},
                    bool track_masters = true, bool track_slaves = true);
  ~SubscribeSentinel() override;

  static std::shared_ptr<SubscribeSentinel> Create(
      const std::shared_ptr<ThreadPools>& thread_pools,
      const secdist::RedisSettings& settings, std::string shard_group_name,
      const std::string& client_name, bool is_cluster_mode,
      const testsuite::RedisControl& testsuite_redis_control);
  static std::shared_ptr<SubscribeSentinel> Create(
      const std::shared_ptr<ThreadPools>& thread_pools,
      const secdist::RedisSettings& settings, std::string shard_group_name,
      const std::string& client_name, ReadyChangeCallback ready_callback,
      bool is_cluster_mode,
      const testsuite::RedisControl& testsuite_redis_control);

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
