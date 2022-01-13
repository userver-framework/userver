#pragma once

#include <memory>

USERVER_NAMESPACE_BEGIN

namespace clients::http {
class Client;
}  // namespace clients::http

namespace engine {
class TaskProcessor;
}

namespace utest {

std::shared_ptr<clients::http::Client> CreateHttpClient();

std::shared_ptr<clients::http::Client> CreateHttpClient(
    engine::TaskProcessor& fs_task_processor);

}  // namespace utest

USERVER_NAMESPACE_END
