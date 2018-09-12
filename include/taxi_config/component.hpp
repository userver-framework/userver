#pragma once

#include <chrono>
#include <memory>

#include <components/caching_component_base.hpp>
#include <components/component_config.hpp>
#include <components/component_context.hpp>
#include <components/updating_component_base.hpp>
#include <taxi_config/config.hpp>

namespace components {

class TaxiConfigImpl;

class TaxiConfig : public CachingComponentBase<taxi_config::Config> {
 public:
  static constexpr const char* kName = "taxi-config";

  TaxiConfig(const ComponentConfig&, const ComponentContext&);
  ~TaxiConfig();

 private:
  void Update(UpdatingComponentBase::UpdateType type,
              const std::chrono::system_clock::time_point& last_update,
              const std::chrono::system_clock::time_point& now) override;

  std::unique_ptr<TaxiConfigImpl> impl_;
};

}  // namespace components
