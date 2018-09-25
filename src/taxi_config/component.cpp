#include <taxi_config/component.hpp>

#include "component_impl.hpp"

namespace components {

TaxiConfig::TaxiConfig(const ComponentConfig& config,
                       const ComponentContext& context)
    : CachingComponentBase<taxi_config::Config>(config, context, kName),
      impl_{std::make_unique<TaxiConfigImpl>(
          config, context, [this](taxi_config::DocsMap&& mongo_docs) {
            this->Emplace(std::move(mongo_docs));
          })} {
  this->StartPeriodicUpdates();
}

TaxiConfig::~TaxiConfig() { this->StopPeriodicUpdates(); }

void TaxiConfig::Update(
    CacheUpdateTrait::UpdateType type,
    const std::chrono::system_clock::time_point& last_update,
    const std::chrono::system_clock::time_point& now) {
  impl_->Update(type, last_update, now);
}

}  // namespace components
