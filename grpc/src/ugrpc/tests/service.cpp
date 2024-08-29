#include <userver/ugrpc/tests/service.hpp>

#include <fmt/format.h>

#include <ugrpc/client/middlewares/deadline_propagation/middleware.hpp>
#include <ugrpc/client/middlewares/log/middleware.hpp>
#include <ugrpc/server/middlewares/deadline_propagation/middleware.hpp>
#include <ugrpc/server/middlewares/log/middleware.hpp>
#include <userver/engine/task/task.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::tests {

ServiceBase::ServiceBase() : ServiceBase(server::ServerConfig{}) {}

ServiceBase::ServiceBase(server::ServerConfig&& server_config)
    : config_storage_(dynamic_config::MakeDefaultStorage({})),
      server_(std::move(server_config), statistics_storage_,
              config_storage_.GetSource()),
      testsuite_({}, false) {}

ServiceBase::~ServiceBase() = default;

void ServiceBase::RegisterService(server::ServiceBase& service) {
  server_.AddService(service, MakeServiceConfig());
}

void ServiceBase::RegisterService(server::GenericServiceBase& service) {
  server_.AddService(service, MakeServiceConfig());
}

server::ServiceConfig ServiceBase::MakeServiceConfig() {
  middlewares_change_allowed_ = false;
  return server::ServiceConfig{
      engine::current_task::GetTaskProcessor(),
      server_middlewares_,
  };
}

void ServiceBase::StartServer(
    client::ClientFactorySettings&& client_factory_settings) {
  middlewares_change_allowed_ = false;
  server_.Start();
  endpoint_ = fmt::format("[::1]:{}", server_.GetPort());
  client_factory_.emplace(std::move(client_factory_settings),
                          engine::current_task::GetTaskProcessor(),
                          client_middleware_factories_,
                          server_.GetCompletionQueue(), statistics_storage_,
                          testsuite_, config_storage_.GetSource());
}

void ServiceBase::StopServer() noexcept {
  client_factory_.reset();
  endpoint_.reset();
  server_.Stop();
}

void ServiceBase::ExtendDynamicConfig(
    const std::vector<dynamic_config::KeyValue>& overrides) {
  config_storage_.Extend(overrides);
}

utils::statistics::Storage& ServiceBase::GetStatisticsStorage() {
  return statistics_storage_;
}

server::Server& ServiceBase::GetServer() noexcept { return server_; }

dynamic_config::Source ServiceBase::GetConfigSource() const {
  return config_storage_.GetSource();
}

void ServiceBase::SetServerMiddlewares(server::Middlewares middlewares) {
  UINVARIANT(middlewares_change_allowed_,
             "Set server middlewares after RegisterService call "
             "is not allowed");
  server_middlewares_ = std::move(middlewares);
}

void ServiceBase::SetClientMiddlewareFactories(
    client::MiddlewareFactories middleware_factories) {
  UINVARIANT(middlewares_change_allowed_,
             "Set client middleware factories after StartServer call "
             "is not allowed");
  client_middleware_factories_ = std::move(middleware_factories);
}

server::Middlewares GetDefaultServerMiddlewares() {
  return {std::make_shared<
      server::middlewares::deadline_propagation::Middleware>()};
}

client::MiddlewareFactories GetDefaultClientMiddlewareFactories() {
  return {std::make_shared<client::middlewares::log::MiddlewareFactory>(
              client::middlewares::log::Settings{}),
          std::make_shared<
              client::middlewares::deadline_propagation::MiddlewareFactory>()};
}

}  // namespace ugrpc::tests

USERVER_NAMESPACE_END
