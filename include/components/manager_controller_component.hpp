#pragma once

#include <components/component_base.hpp>
#include <components/component_config.hpp>
#include <components/component_context.hpp>
#include <utils/statistics/storage.hpp>

#include <taxi_config/storage/component.hpp>

namespace engine {
class TaskProcessor;
}  // namespace engine

namespace components {

class Manager;

class ManagerControllerComponent : public ComponentBase {
 public:
  ManagerControllerComponent(const components::ComponentConfig& config,
                             const components::ComponentContext& context);

  ~ManagerControllerComponent();

  static constexpr const char* kName = "manager-controller";

 private:
  formats::json::ValueBuilder GetTaskProcessorStats(
      const engine::TaskProcessor& task_processor) const;

  formats::json::Value ExtendStatistics(
      const utils::statistics::StatisticsRequest& /*request*/);

  using TaxiConfigPtr = std::shared_ptr<taxi_config::Config>;
  void OnConfigUpdate(const TaxiConfigPtr& cfg);

 private:
  const components::Manager& components_manager_;
  components::ComponentContext::TaskProcessorPtrMap task_processor_map_;
  utils::statistics::Entry statistics_holder_;
  utils::AsyncEventSubscriberScope config_subscription_;
};  // namespace components

}  // namespace components
