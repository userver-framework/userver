#include <server/component.hpp>

#include <components/statistics_storage.hpp>
#include <server/server_config.hpp>

namespace components {

namespace {
const auto kStatisticsName = "server";
}  // namespace

Server::Server(const components::ComponentConfig& component_config,
               const components::ComponentContext& component_context)
    : LoggableComponentBase(component_config, component_context),
      server_(std::make_unique<server::Server>(
          component_config.As<server::ServerConfig>(), component_context)),
      statistics_storage_(
          component_context.FindComponent<StatisticsStorage>()) {
  statistics_holder_ = statistics_storage_.GetStorage().RegisterExtender(
      kStatisticsName,
      std::bind(&Server::ExtendStatistics, this, std::placeholders::_1));
}

Server::~Server() { statistics_holder_.Unregister(); }

void Server::OnAllComponentsLoaded() { server_->Start(); }

void Server::OnAllComponentsAreStopping() {
  /* components::Server has to stop all Listeners before unloading components
   * as handlers have no ability to call smth like RemoveHandler() from
   * server::Server. Without such server stop before unloading a new request may
   * use a handler while the handler is destroying.
   */
  server_->Stop();
}

const server::Server& Server::GetServer() const { return *server_; }

server::Server& Server::GetServer() { return *server_; }

void Server::AddHandler(const server::handlers::HttpHandlerBase& handler,
                        engine::TaskProcessor& task_processor) {
  server_->AddHandler(handler, task_processor);
}

formats::json::Value Server::ExtendStatistics(
    const utils::statistics::StatisticsRequest& request) {
  return server_->GetMonitorData(request);
}

}  // namespace components
