#include <userver/clients/http/client.hpp>
#include <userver/engine/task/task.hpp>
#include <userver/utest/http_client.hpp>

USERVER_NAMESPACE_BEGIN

namespace utest {

std::shared_ptr<clients::http::Client> CreateHttpClient() {
  return std::make_shared<clients::http::Client>(
      "", 1, engine::current_task::GetTaskProcessor());
}

}  // namespace utest

USERVER_NAMESPACE_END
