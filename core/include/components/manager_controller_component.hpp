#pragma once

#include <components/component_config.hpp>
#include <components/component_context.hpp>
#include <components/impl/component_base.hpp>
#include <engine/task/task_processor_fwd.hpp>
#include <utils/statistics/storage.hpp>

#include <taxi_config/storage/component.hpp>

namespace components {

class Manager;

class ManagerControllerComponent final : public impl::ComponentBase {
 public:
  ManagerControllerComponent(const components::ComponentConfig& config,
                             const components::ComponentContext& context);

  ~ManagerControllerComponent() override;

  static constexpr const char* kName = "manager-controller";

 private:
  formats::json::Value ExtendStatistics(
      const utils::statistics::StatisticsRequest& /*request*/);

  using TaxiConfigPtr = std::shared_ptr<const taxi_config::Config>;
  void OnConfigUpdate(const TaxiConfigPtr& cfg);

 private:
  const components::Manager& components_manager_;
  utils::statistics::Entry statistics_holder_;
  utils::AsyncEventSubscriberScope config_subscription_;
};

}  // namespace components
