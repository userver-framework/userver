#pragma once

#include <chrono>

#include <components/caching_component_base.hpp>
#include <components/component_config.hpp>
#include <components/component_context.hpp>
#include <components/updating_component_base.hpp>

#include "component_impl.hpp"

namespace components {

template <typename Config>
class TaxiConfig : public CachingComponentBase<Config> {
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

template <typename Config>
TaxiConfig<Config>::TaxiConfig(const ComponentConfig& config,
                               const ComponentContext& context)
    : CachingComponentBase<Config>(config, kName),
      impl_(config, context, [this](taxi_config::DocsMap&& mongo_docs) {
        this->Emplace(std::move(mongo_docs));
      }) {
  this->StartPeriodicUpdates();
}

template <typename Config>
TaxiConfig<Config>::~TaxiConfig() {
  this->StopPeriodicUpdates();
}

template <typename Config>
void TaxiConfig<Config>::Update(
    UpdatingComponentBase::UpdateType type,
    const std::chrono::system_clock::time_point& last_update,
    const std::chrono::system_clock::time_point& now) {
  impl_.Update(type, last_update, now);
}

}  // namespace components
