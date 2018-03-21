#include "handler_base.hpp"

namespace components {
namespace handlers {

HandlerBase::HandlerBase(const ComponentConfig& config,
                         const ComponentContext& /*component_context*/) {
  config_ = HandlerConfig::ParseFromJson(config.Json(), config.FullPath(),
                                         config.ConfigVarsPtr());
}

const HandlerConfig& HandlerBase::GetConfig() const { return config_; }

}  // namespace handlers
}  // namespace components
