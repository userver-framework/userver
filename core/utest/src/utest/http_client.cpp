#include <userver/clients/http/client.hpp>

#include <userver/clients/http/config.hpp>
#include <userver/engine/task/task.hpp>
#include <userver/tracing/manager.hpp>
#include <userver/utest/http_client.hpp>

USERVER_NAMESPACE_BEGIN

namespace utest {
namespace {

std::shared_ptr<clients::http::Client> Create(
    engine::TaskProcessor& fs_task_processor,
    const tracing::TracingManagerBase& tracing_manager,
    clients::http::Plugin* plugin
) {
    clients::http::ClientSettings static_config;
    static_config.io_threads = 1;
    static_config.tracing_manager = &tracing_manager;

    std::vector<utils::NotNull<clients::http::Plugin*>> plugins;
    if (plugin) plugins.emplace_back(plugin);

    return std::make_shared<clients::http::Client>(std::move(static_config), fs_task_processor, plugins);
}

const tracing::GenericTracingManager& GetDefaultTracingManager() {
    static const tracing::GenericTracingManager kDefaultTracingManager{
        tracing::Format::kYandexTaxi, tracing::Format::kYandexTaxi};
    return kDefaultTracingManager;
}

}  // namespace

std::shared_ptr<clients::http::Client> CreateHttpClient() {
    return utest::CreateHttpClient(engine::current_task::GetTaskProcessor());
}

std::shared_ptr<clients::http::Client> CreateHttpClient(engine::TaskProcessor& fs_task_processor) {
    return Create(fs_task_processor, GetDefaultTracingManager(), {});
}

std::shared_ptr<clients::http::Client> CreateHttpClientWithPlugin(clients::http::Plugin& plugin) {
    return Create(engine::current_task::GetTaskProcessor(), GetDefaultTracingManager(), &plugin);
}

std::shared_ptr<clients::http::Client> CreateHttpClient(const tracing::TracingManagerBase& tracing_manager) {
    return Create(engine::current_task::GetTaskProcessor(), tracing_manager, {});
}

}  // namespace utest

USERVER_NAMESPACE_END
