#include <taxi_config/component.hpp>

namespace components {

TaxiConfig::TaxiConfig(const ComponentConfig& config,
                       const ComponentContext& context)
    : CachingComponentBase<taxi_config::Config>(config, context, kName),
      impl_(config, context, [this](taxi_config::DocsMap&& mongo_docs) {
        this->Emplace(std::move(mongo_docs));
      }) {
  this->StartPeriodicUpdates();
}

TaxiConfig::~TaxiConfig() { this->StopPeriodicUpdates(); }

void TaxiConfig::Update(
    UpdatingComponentBase::UpdateType type,
    const std::chrono::system_clock::time_point& last_update,
    const std::chrono::system_clock::time_point& now) {
  impl_.Update(type, last_update, now);
}

}  // namespace components
