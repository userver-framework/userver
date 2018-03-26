#include "handler_base.hpp"

namespace server {
namespace handlers {

HandlerBase::HandlerBase(
    const components::ComponentConfig& config,
    const components::ComponentContext& /*component_context*/) {
  config_ = HandlerConfig::ParseFromJson(config.Json(), config.FullPath(),
                                         config.ConfigVarsPtr());
}

const HandlerConfig& HandlerBase::GetConfig() const { return config_; }

}  // namespace handlers
}  // namespace server
