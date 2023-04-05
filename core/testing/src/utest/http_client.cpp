#include <userver/clients/http/client.hpp>
#include <userver/engine/task/task.hpp>
#include <userver/utest/http_client.hpp>

USERVER_NAMESPACE_BEGIN

namespace utest {

std::shared_ptr<clients::http::Client> CreateHttpClient() {
  return CreateHttpClient(engine::current_task::GetTaskProcessor());
}
std::shared_ptr<clients::http::Client> CreateHttpClient(
    engine::TaskProcessor& fs_task_processor) {
  return std::make_shared<clients::http::Client>(
      clients::http::ClientSettings{"", 1, false}, fs_task_processor,
      std::vector<utils::NotNull<clients::http::Plugin*>>{});
}

}  // namespace utest

USERVER_NAMESPACE_END
