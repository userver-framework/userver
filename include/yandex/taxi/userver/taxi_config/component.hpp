#pragma once

#include <chrono>

#include <yandex/taxi/userver/components/caching_component_base.hpp>
#include <yandex/taxi/userver/components/component_config.hpp>
#include <yandex/taxi/userver/components/component_context.hpp>
#include <yandex/taxi/userver/components/updating_component_base.hpp>
#include <yandex/taxi/userver/taxi_config/taxi_config.hpp>

#include "../../../../../src/taxi_config/component_impl.hpp"

namespace components {

class TaxiConfig : public CachingComponentBase<taxi_config::Config> {
 public:
  static constexpr const char* kName = "taxi-config";

  TaxiConfig(const ComponentConfig&, const ComponentContext&);
  ~TaxiConfig();

 private:
  void Update(UpdatingComponentBase::UpdateType type,
              const std::chrono::system_clock::time_point& last_update,
              const std::chrono::system_clock::time_point& now) override;

  TaxiConfigImpl impl_;
};

}  // namespace components
