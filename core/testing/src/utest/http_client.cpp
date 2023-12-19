#include <userver/clients/http/client.hpp>

#include <userver/clients/http/impl/config.hpp>
#include <userver/engine/task/task.hpp>
#include <userver/tracing/manager.hpp>
#include <userver/utest/http_client.hpp>

USERVER_NAMESPACE_BEGIN

namespace utest {

std::shared_ptr<clients::http::Client> CreateHttpClient() {
  return utest::CreateHttpClient(engine::current_task::GetTaskProcessor());
}

std::shared_ptr<clients::http::Client> CreateHttpClient(
    engine::TaskProcessor& fs_task_processor) {
  static const tracing::GenericTracingManager kDefaultTracingManager{
      tracing::Format::kYandexTaxi, tracing::Format::kYandexTaxi};

  clients::http::impl::ClientSettings static_config;
  static_config.io_threads = 1;
  static_config.tracing_manager = &kDefaultTracingManager;

  return std::make_shared<clients::http::Client>(
      std::move(static_config), fs_task_processor,
      std::vector<utils::NotNull<clients::http::Plugin*>>{});
}

std::shared_ptr<clients::http::Client> CreateHttpClient(
    const tracing::TracingManagerBase& tracing_manager) {
  clients::http::impl::ClientSettings static_config;
  static_config.io_threads = 1;
  static_config.tracing_manager = &tracing_manager;

  return std::make_shared<clients::http::Client>(
      std::move(static_config), engine::current_task::GetTaskProcessor(),
      std::vector<utils::NotNull<clients::http::Plugin*>>{});
}

}  // namespace utest

USERVER_NAMESPACE_END
