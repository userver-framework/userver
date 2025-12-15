#include <userver/utest/http_client.hpp>

#include <userver/clients/http/client_core.hpp>
#include <userver/clients/http/client_with_middlewares.hpp>
#include <userver/clients/http/config.hpp>
#include <userver/engine/task/task.hpp>
#include <userver/tracing/manager.hpp>

USERVER_NAMESPACE_BEGIN

namespace utest {
namespace {

std::shared_ptr<clients::http::ClientCore> CreateCore(
    engine::TaskProcessor& fs_task_processor,
    const tracing::TracingManagerBase& tracing_manager
) {
    clients::http::ClientSettings static_config;
    static_config.io_threads = 1;
    static_config.tracing_manager = &tracing_manager;

    return std::make_shared<
        clients::http::ClientCore>(utils::impl::InternalTag{}, std::move(static_config), fs_task_processor);
}

std::shared_ptr<clients::http::Client> Create(
    engine::TaskProcessor& fs_task_processor,
    const tracing::TracingManagerBase& tracing_manager,
    clients::http::MiddlewareBase* middleware
) {
    std::vector<utils::NotNull<clients::http::MiddlewareBase*>> middlewares;
    if (middleware) {
        middlewares.emplace_back(middleware);
    }

    return std::make_shared<clients::http::ClientWithMiddlewares>(
        utils::impl::InternalTag{},
        CreateCore(fs_task_processor, tracing_manager),
        middlewares
    );
}

const tracing::GenericTracingManager& GetDefaultTracingManager() {
    static const tracing::GenericTracingManager
        kDefaultTracingManager{tracing::Format::kYandexTaxi, tracing::Format::kYandexTaxi};
    return kDefaultTracingManager;
}

}  // namespace

namespace impl {

std::shared_ptr<clients::http::ClientCore> CreateHttpClientCore() {
    return CreateHttpClientCore(engine::current_task::GetTaskProcessor());
}

std::shared_ptr<clients::http::ClientCore> CreateHttpClientCore(engine::TaskProcessor& fs_task_processor) {
    return CreateCore(fs_task_processor, GetDefaultTracingManager());
}

std::shared_ptr<clients::http::ClientWithMiddlewares> CreateHttpClientWithMiddlewares(
    engine::TaskProcessor& fs_task_processor
) {
    return std::make_shared<clients::http::ClientWithMiddlewares>(
        utils::impl::InternalTag{},
        CreateCore(fs_task_processor, GetDefaultTracingManager()),
        std::vector<utils::NotNull<clients::http::MiddlewareBase*>>{}
    );
}

}  // namespace impl

std::shared_ptr<clients::http::Client> CreateHttpClient() {
    return utest::CreateHttpClient(engine::current_task::GetTaskProcessor());
}

std::shared_ptr<clients::http::Client> CreateHttpClient(engine::TaskProcessor& fs_task_processor) {
    return Create(fs_task_processor, GetDefaultTracingManager(), {});
}

std::shared_ptr<clients::http::Client> CreateHttpClientWithMiddleware(clients::http::MiddlewareBase& middleware) {
    return Create(engine::current_task::GetTaskProcessor(), GetDefaultTracingManager(), &middleware);
}

std::shared_ptr<clients::http::Client> CreateHttpClient(const tracing::TracingManagerBase& tracing_manager) {
    return Create(engine::current_task::GetTaskProcessor(), tracing_manager, {});
}

}  // namespace utest

USERVER_NAMESPACE_END
