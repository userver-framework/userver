#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include <components/component_base.hpp>
#include <components/component_config.hpp>
#include <components/component_context.hpp>

#include <taxi_config/component.hpp>

namespace redis {
class Sentinel;
class ThreadPools;
}  // namespace redis

namespace components {

class Redis : public ComponentBase {
 public:
  Redis(const ComponentConfig& config,
        const ComponentContext& component_context);

  ~Redis();

  static constexpr const char* kName = "redis";

  std::shared_ptr<redis::Sentinel> Client(const std::string& name) {
    return clients_.at(name);
  }

 private:
  using TaxiConfigPtr = std::shared_ptr<taxi_config::Config>;
  void OnConfigUpdate(const TaxiConfigPtr& cfg);

  std::unordered_map<std::string, std::shared_ptr<redis::Sentinel>> clients_;
  std::shared_ptr<redis::ThreadPools> thread_pools_;

  TaxiConfig* const config_;
  utils::AsyncEventSubscriberScope config_subscription_;
};

}  // namespace components
