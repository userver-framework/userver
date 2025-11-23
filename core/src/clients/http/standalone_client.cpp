#include <userver/clients/http/standalone_client.hpp>

#include <userver/clients/http/client_core.hpp>
#include <userver/clients/http/client_with_plugins.hpp>

USERVER_NAMESPACE_BEGIN

namespace clients::http {

std::shared_ptr<Client> CreateStandaloneHttpClient(
    ClientSettings settings,
    StandaloneConfig standalone_config,
    engine::TaskProcessor& fs_task_processor,
    std::vector<utils::NotNull<clients::http::Plugin*>> plugins
) {
    auto core_client = std::make_shared<ClientCore>(utils::impl::InternalTag{}, std::move(settings), fs_task_processor);
    core_client->SetMultiplexingEnabled(standalone_config.multiplexing_enabled);
    if (standalone_config.max_host_connections) {
        core_client->SetMaxHostConnections(*standalone_config.max_host_connections);
    }
    return std::make_shared<ClientWithPlugins>(utils::impl::InternalTag{}, core_client, std::move(plugins));
}

}  // namespace clients::http

USERVER_NAMESPACE_END
