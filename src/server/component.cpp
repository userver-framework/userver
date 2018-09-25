#include <server/component.hpp>

#include <components/statistics_storage.hpp>
#include <server/server_config.hpp>

namespace components {

namespace {
const auto kStatisticsName = "server";
}  // namespace

Server::Server(const components::ComponentConfig& component_config,
               const components::ComponentContext& component_context)
    : server_(std::make_unique<server::Server>(
          server::ServerConfig::ParseFromJson(component_config.Json(),
                                              component_config.FullPath(),
                                              component_config.ConfigVarsPtr()),
          component_context)),
      statistics_storage_(
          component_context.FindComponentRequired<StatisticsStorage>()) {
  statistics_holder_ = statistics_storage_->GetStorage().RegisterExtender(
      kStatisticsName,
      std::bind(&Server::ExtendStatistics, this, std::placeholders::_1));
}

Server::~Server() { statistics_holder_.Unregister(); }

void Server::OnAllComponentsLoaded() { server_->Start(); }

const server::Server& Server::GetServer() const { return *server_; }

bool Server::AddHandler(const server::handlers::HandlerBase& handler,
                        const components::ComponentContext& component_context) {
  return server_->AddHandler(handler, component_context);
}

formats::json::Value Server::ExtendStatistics(
    const utils::statistics::StatisticsRequest& request) {
  return server_->GetMonitorData(request);
}

}  // namespace components
