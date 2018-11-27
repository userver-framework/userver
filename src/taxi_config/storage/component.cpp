#include <taxi_config/storage/component.hpp>

#include <fstream>

#include <blocking/fs/read.hpp>

namespace components {

TaxiConfig::TaxiConfig(const ComponentConfig& config,
                       const ComponentContext& context)
    : LoggableComponentBase(config, context), config_load_cancelled_(false) {
  try {
    auto bootstrap_config_contents =
        blocking::fs::ReadFileContents(config.ParseString("bootstrap_path"));
    taxi_config::DocsMap bootstrap_config;
    bootstrap_config.Parse(bootstrap_config_contents, false);
    bootstrap_config_ =
        std::make_shared<taxi_config::BootstrapConfig>(bootstrap_config);
  } catch (const std::exception& ex) {
    throw std::runtime_error(
        std::string("Cannot load bootstrap taxi config: ") + ex.what());
  }
}

TaxiConfig::~TaxiConfig() {}

std::shared_ptr<taxi_config::Config> TaxiConfig::Get() const {
  auto ptr = cache_.Get();
  if (ptr) return ptr;

  LOG_TRACE() << "Wait started";
  std::unique_lock<engine::Mutex> lock(loaded_mutex_);
  loaded_cv_.Wait(lock, [this]() {
    return cache_.Get() != nullptr || config_load_cancelled_;
  });
  LOG_TRACE() << "Wait finished";

  if (config_load_cancelled_) throw std::runtime_error("config load cancelled");
  return cache_.Get();
}

std::shared_ptr<taxi_config::BootstrapConfig> TaxiConfig::GetBootstrap() const {
  return bootstrap_config_;
}

void TaxiConfig::Set(std::shared_ptr<taxi_config::Config> value_ptr) {
  cache_.Set(std::move(value_ptr));
  loaded_cv_.NotifyAll();
  SendEvent(cache_.Get());
}

void TaxiConfig::SetLoadingFailed() {
  LOG_ERROR() << "Config was set to nullptr, config load cancelled";
  config_load_cancelled_ = true;
  loaded_cv_.NotifyAll();
}

}  // namespace components
