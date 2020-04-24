#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include <components/component_config.hpp>
#include <components/component_context.hpp>
#include <storages/redis/impl/wait_connected_mode.hpp>
#include <utils/statistics/storage.hpp>

#include <testsuite/testsuite_support.hpp>

#include <taxi_config/storage/component.hpp>

namespace redis {
class Sentinel;
class ThreadPools;
}  // namespace redis

namespace storages {
namespace redis {
class Client;
class SubscribeClient;
class SubscribeClientImpl;
}  // namespace redis
}  // namespace storages

namespace components {

class StatisticsStorage;

class Redis : public LoggableComponentBase {
 public:
  Redis(const ComponentConfig& config,
        const ComponentContext& component_context);

  ~Redis() override;

  static constexpr const char* kName = "redis";

  std::shared_ptr<storages::redis::Client> GetClient(
      const std::string& name,
      ::redis::RedisWaitConnected wait_connected = {}) const;
  [[deprecated("use GetClient()")]] std::shared_ptr<redis::Sentinel> Client(
      const std::string& name) const;
  std::shared_ptr<storages::redis::SubscribeClient> GetSubscribeClient(
      const std::string& name,
      ::redis::RedisWaitConnected wait_connected = {}) const;

 private:
  using TaxiConfigPtr = std::shared_ptr<const taxi_config::Config>;
  void OnConfigUpdate(const TaxiConfigPtr& cfg);

  void Connect(const ComponentConfig& config,
               const ComponentContext& component_context,
               const testsuite::RedisControl& testsuite_redis_control);

  formats::json::Value ExtendStatisticsRedis(
      const utils::statistics::StatisticsRequest& /*request*/);

  formats::json::Value ExtendStatisticsRedisPubsub(
      const utils::statistics::StatisticsRequest& /*request*/);

  std::shared_ptr<redis::ThreadPools> thread_pools_;
  std::unordered_map<std::string, std::shared_ptr<redis::Sentinel>> sentinels_;
  std::unordered_map<std::string, std::shared_ptr<storages::redis::Client>>
      clients_;
  std::unordered_map<std::string,
                     std::shared_ptr<storages::redis::SubscribeClientImpl>>
      subscribe_clients_;

  TaxiConfig& config_;
  utils::AsyncEventSubscriberScope config_subscription_;

  components::StatisticsStorage& statistics_storage_;
  utils::statistics::Entry statistics_holder_;
  utils::statistics::Entry subscribe_statistics_holder_;
};

}  // namespace components
