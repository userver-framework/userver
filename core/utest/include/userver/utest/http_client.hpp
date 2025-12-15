#pragma once

#include <memory>

#include <userver/engine/task/current_task.hpp>

USERVER_NAMESPACE_BEGIN

namespace clients::http {
class ClientCore;
class Client;
class ClientWithMiddlewares;
class MiddlewareBase;
}  // namespace clients::http

namespace engine {
class TaskProcessor;
}

namespace tracing {
class TracingManagerBase;
}

namespace utest {

namespace impl {

std::shared_ptr<clients::http::ClientCore> CreateHttpClientCore();

std::shared_ptr<clients::http::ClientCore> CreateHttpClientCore(engine::TaskProcessor& fs_task_processor);

std::shared_ptr<clients::http::ClientWithMiddlewares> CreateHttpClientWithMiddlewares(
    engine::TaskProcessor& fs_task_processor = engine::current_task::GetBlockingTaskProcessor()
);

}  // namespace impl

std::shared_ptr<clients::http::Client> CreateHttpClient();

std::shared_ptr<clients::http::Client> CreateHttpClient(engine::TaskProcessor& fs_task_processor);

std::shared_ptr<clients::http::Client> CreateHttpClientWithMiddleware(clients::http::MiddlewareBase&);

std::shared_ptr<clients::http::Client> CreateHttpClient(const tracing::TracingManagerBase& tracing_manager);

}  // namespace utest

USERVER_NAMESPACE_END
