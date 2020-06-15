#include <clients/http/client.hpp>
#include <engine/task/task.hpp>
#include <utest/http_client.hpp>

namespace utest {

std::shared_ptr<clients::http::Client> CreateHttpClient() {
  return clients::http::Client::Create(
      "", 1, engine::current_task::GetTaskProcessor());
}

}  // namespace utest
