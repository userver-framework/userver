#include "component.hpp"

#include "server_config.hpp"

namespace components {

Server::Server(const components::ComponentConfig& component_config,
               const components::ComponentContext& component_context)
    : server_(std::make_unique<server::Server>(
          server::ServerConfig::ParseFromJson(component_config.Json(),
                                              component_config.FullPath(),
                                              component_config.ConfigVarsPtr()),
          component_context)) {}

const server::Server& Server::GetServer() const { return *server_; }

Json::Value Server::GetMonitorData(MonitorVerbosity verbosity) const {
  return server_->GetMonitorData(verbosity);
}

}  // namespace components
