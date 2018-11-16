#include <taxi_config/storage/component.hpp>

namespace components {

TaxiConfig::TaxiConfig(const ComponentConfig& config,
                       const ComponentContext& context)
    : LoggableComponentBase(config, context) {}

TaxiConfig::~TaxiConfig() {}

std::shared_ptr<taxi_config::Config> TaxiConfig::Get() const {
  auto ptr = cache_.Get();
  if (ptr) return ptr;

  LOG_INFO() << "Wait started";
  std::unique_lock<engine::Mutex> lock(loaded_mutex_);
  loaded_cv_.Wait(lock, [this]() { return cache_.Get() != nullptr; });
  LOG_INFO() << "Wait finished";
  return cache_.Get();
}

void TaxiConfig::Set(std::shared_ptr<taxi_config::Config> value_ptr) {
  LOG_INFO() << "Set";
  cache_.Set(std::move(value_ptr));
  loaded_cv_.NotifyAll();
  SendEvent(Get());
}

}  // namespace components
