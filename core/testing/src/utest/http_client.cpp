#include <userver/clients/http/client.hpp>

#include <userver/clients/http/impl/config.hpp>
#include <userver/engine/task/task.hpp>
#include <userver/utest/http_client.hpp>

USERVER_NAMESPACE_BEGIN

namespace utest {

std::shared_ptr<clients::http::Client> CreateHttpClient() {
  return CreateHttpClient(engine::current_task::GetTaskProcessor());
}
std::shared_ptr<clients::http::Client> CreateHttpClient(
    engine::TaskProcessor& fs_task_processor) {
  clients::http::impl::ClientSettings static_config;
  static_config.io_threads = 1;
  return std::make_shared<clients::http::Client>(
      std::move(static_config), fs_task_processor,
      std::vector<utils::NotNull<clients::http::Plugin*>>{});
}

}  // namespace utest

USERVER_NAMESPACE_END
