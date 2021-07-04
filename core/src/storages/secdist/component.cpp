#include <userver/storages/secdist/component.hpp>

#include <userver/logging/log.hpp>

#include <userver/storages/secdist/exceptions.hpp>

namespace components {

Secdist::Secdist(const ComponentConfig& config, const ComponentContext& context)
    : LoggableComponentBase(config, context) {
  const auto config_path = config["config"].As<std::string>();
  bool missing_ok = config["missing-ok"].As<bool>(false);
  secdist_config_ = storages::secdist::SecdistConfig(config_path, missing_ok);
}

const storages::secdist::SecdistConfig& Secdist::Get() const {
  return secdist_config_;
}

}  // namespace components
