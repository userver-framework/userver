#include "component.hpp"

#include <logging/log.hpp>

namespace components {

Secdist::Secdist(const ComponentConfig& config, const ComponentContext&) {
  auto config_path = config.ParseString("config");

  try {
    secdist_config_ = storages::secdist::SecdistConfig(config_path);
  } catch (const storages::secdist::SecdistError& ex) {
    LOG_ERROR() << "Failed to load secdist config from " << config_path << ": "
                << ex.what();
    throw;
  }
}

const storages::secdist::SecdistConfig& Secdist::Get() const {
  return secdist_config_;
}

}  // namespace components
