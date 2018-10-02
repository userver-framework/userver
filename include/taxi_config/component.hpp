#pragma once

#include <chrono>
#include <memory>

#include <components/cache_update_trait.hpp>
#include <components/caching_component_base.hpp>
#include <components/component_config.hpp>
#include <components/component_context.hpp>
#include <taxi_config/config.hpp>

namespace components {

class TaxiConfigImpl;

class TaxiConfig : public CachingComponentBase<taxi_config::Config> {
 public:
  static constexpr const char* kName = "taxi-config";

  TaxiConfig(const ComponentConfig&, const ComponentContext&);
  ~TaxiConfig();

 private:
  void Update(CacheUpdateTrait::UpdateType type,
              const std::chrono::system_clock::time_point& last_update,
              const std::chrono::system_clock::time_point& now,
              tracing::Span&& span) override;

  std::unique_ptr<TaxiConfigImpl> impl_;
};

}  // namespace components
