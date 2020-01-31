#include <storages/secdist/component.hpp>

#include <logging/log.hpp>

#include <storages/secdist/exceptions.hpp>

namespace components {

Secdist::Secdist(const ComponentConfig& config, const ComponentContext& context)
    : LoggableComponentBase(config, context) {
  auto config_path = config.ParseString("config");
  secdist_config_ = storages::secdist::SecdistConfig(config_path);
}

const storages::secdist::SecdistConfig& Secdist::Get() const {
  return secdist_config_;
}

}  // namespace components
