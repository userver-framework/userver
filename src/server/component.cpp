#include <yandex/taxi/userver/server/component.hpp>

#include <yandex/taxi/userver/server/server_config.hpp>

namespace components {

Server::Server(const components::ComponentConfig& component_config,
               const components::ComponentContext& component_context)
    : server_(std::make_unique<server::Server>(
          server::ServerConfig::ParseFromJson(component_config.Json(),
                                              component_config.FullPath(),
                                              component_config.ConfigVarsPtr()),
          component_context)) {}

void Server::OnAllComponentsLoaded() { server_->Start(); }

const server::Server& Server::GetServer() const { return *server_; }

bool Server::AddHandler(const server::handlers::HandlerBase& handler,
                        const components::ComponentContext& component_context) {
  return server_->AddHandler(handler, component_context);
}

Json::Value Server::GetMonitorData(MonitorVerbosity verbosity) const {
  return server_->GetMonitorData(verbosity);
}

}  // namespace components
