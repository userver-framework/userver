#pragma once

#include <memory>

USERVER_NAMESPACE_BEGIN

namespace clients::http {
class Client;
}  // namespace clients::http

namespace engine {
class TaskProcessor;
}

namespace tracing {
class TracingManagerBase;
}

namespace utest {

std::shared_ptr<clients::http::Client> CreateHttpClient();

std::shared_ptr<clients::http::Client> CreateHttpClient(
    engine::TaskProcessor& fs_task_processor);

std::shared_ptr<clients::http::Client> CreateHttpClient(
    const tracing::TracingManagerBase& tracing_manager);

}  // namespace utest

USERVER_NAMESPACE_END
