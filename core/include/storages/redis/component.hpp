#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include <components/component_config.hpp>
#include <components/component_context.hpp>
#include <utils/statistics/storage.hpp>

#include <taxi_config/storage/component.hpp>

namespace redis {
class Sentinel;
class ThreadPools;
}  // namespace redis

namespace components {

class StatisticsStorage;

class Redis : public LoggableComponentBase {
 public:
  Redis(const ComponentConfig& config,
        const ComponentContext& component_context);

  ~Redis();

  static constexpr const char* kName = "redis";

  std::shared_ptr<redis::Sentinel> Client(const std::string& name) const;

 private:
  using TaxiConfigPtr = std::shared_ptr<taxi_config::Config>;
  void OnConfigUpdate(const TaxiConfigPtr& cfg);

  void Connect(const ComponentConfig& config,
               const ComponentContext& component_context);

  formats::json::Value ExtendStatistics(
      const utils::statistics::StatisticsRequest& /*request*/);

  std::unordered_map<std::string, std::shared_ptr<redis::Sentinel>> clients_;
  std::shared_ptr<redis::ThreadPools> thread_pools_;

  TaxiConfig& config_;
  utils::AsyncEventSubscriberScope config_subscription_;

  components::StatisticsStorage& statistics_storage_;
  utils::statistics::Entry statistics_holder_;
};

}  // namespace components
