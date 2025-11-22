#pragma once

#include <memory>

USERVER_NAMESPACE_BEGIN

namespace clients::http {
class ClientCore;
class Client;
class Plugin;
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

}  // namespace impl

std::shared_ptr<clients::http::Client> CreateHttpClient();

std::shared_ptr<clients::http::Client> CreateHttpClient(engine::TaskProcessor& fs_task_processor);

std::shared_ptr<clients::http::Client> CreateHttpClientWithPlugin(clients::http::Plugin&);

std::shared_ptr<clients::http::Client> CreateHttpClient(const tracing::TracingManagerBase& tracing_manager);

}  // namespace utest

USERVER_NAMESPACE_END
