#include <userver/server/component.hpp>

#include <server/server_config.hpp>
#include <userver/components/component.hpp>
#include <userver/components/statistics_storage.hpp>
#include <userver/utils/resource_scopes.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

#ifndef ARCADIA_ROOT
#include "generated/src/server/component.yaml.hpp"  // Y_IGNORE
#endif

USERVER_NAMESPACE_BEGIN

namespace components {

namespace {
const storages::secdist::SecdistConfig& GetSecdist(const components::ComponentContext& component_context) {
    auto* component = component_context.FindComponentOptional<components::Secdist>();
    if (component) {
        return component->Get();
    }

    static const storages::secdist::SecdistConfig kEmpty;
    return kEmpty;
}
}  // namespace

Server::Server(
    const components::ComponentConfig& component_config,
    const components::ComponentContext& component_context
)
    : ComponentBase(component_config, component_context),
      server_(std::make_unique<server::Server>(
          component_config.As<server::ServerConfig>(),
          GetSecdist(component_context),
          component_context
      ))
{
    // Early start of monitor port, before all components are loaded,
    // the service is not ready yet.
    server_->StartMonitorPort();

    utils::statistics::RegisterWriterScope(component_context, "", [this](auto& writer) {
        server_->WriteMetrics(writer);
    });
}

void Server::OnAllComponentsLoaded() {
    // All handler components are setup with their dependencies, we're ready to handle requests.
    server_->Start();
}

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

void Server::AddHandler(const server::handlers::HttpHandlerBase& handler, engine::TaskProcessor& task_processor) {
    server_->AddHandler(handler, task_processor);
}

void Server::WriteStatistics(utils::statistics::Writer& writer) { server_->WriteMonitorData(writer); }

yaml_config::Schema Server::GetStaticConfigSchema() {
    return yaml_config::MergeSchemasFromResource<ComponentBase>("src/server/component.yaml");
}

}  // namespace components

USERVER_NAMESPACE_END
