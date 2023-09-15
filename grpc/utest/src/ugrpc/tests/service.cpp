#include <userver/ugrpc/tests/service.hpp>

#include <fmt/format.h>

#include <ugrpc/client/middlewares/deadline_propagation/middleware.hpp>
#include <ugrpc/client/middlewares/log/middleware.hpp>
#include <ugrpc/server/middlewares/deadline_propagation/middleware.hpp>
#include <ugrpc/server/middlewares/log/middleware.hpp>
#include <userver/engine/task/task.hpp>
#include <userver/logging/null_logger.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::tests {

namespace {

using ClientLogMiddlewareFactory = client::middlewares::log::MiddlewareFactory;
using ClientLogMiddlewareSettings =
    client::middlewares::log::Middleware::Settings;

using ClientDpMiddlewareFactory =
    client::middlewares::deadline_propagation::MiddlewareFactory;

using ServerLogMiddleware = server::middlewares::log::Middleware;
using ServerLogMiddlewareSettings =
    server::middlewares::log::Middleware::Settings;

using ServerDpMiddleware =
    server::middlewares::deadline_propagation::Middleware;

}  // namespace

ServiceBase::ServiceBase(dynamic_config::StorageMock&& dynconf,
                         server::ServerConfig&& server_config)
    : config_storage_(std::move(dynconf)),
      server_(std::move(server_config), statistics_storage_,
              config_storage_.GetSource()),
      server_middlewares_(
          {std::make_shared<ServerLogMiddleware>(ServerLogMiddlewareSettings{}),
           std::make_shared<ServerDpMiddleware>()}),
      middleware_factories_({std::make_shared<ClientLogMiddlewareFactory>(
                                 ClientLogMiddlewareSettings{}),
                             std::make_shared<ClientDpMiddlewareFactory>()}),
      testsuite_({}, false) {}

ServiceBase::~ServiceBase() = default;

void ServiceBase::RegisterService(server::ServiceBase& service) {
  adding_middlewares_allowed_ = false;
  server_.AddService(service, server::ServiceConfig{
                                  engine::current_task::GetTaskProcessor(),
                                  server_middlewares_,
                              });
}

void ServiceBase::StartServer(
    client::ClientFactoryConfig&& client_factory_config) {
  adding_middlewares_allowed_ = false;
  server_.Start();
  endpoint_ = fmt::format("[::1]:{}", server_.GetPort());
  client_factory_.emplace(std::move(client_factory_config),
                          engine::current_task::GetTaskProcessor(),
                          middleware_factories_, server_.GetCompletionQueue(),
                          statistics_storage_, testsuite_,
                          config_storage_.GetSource());
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

server::Server& ServiceBase::GetServer() noexcept { return server_; }

dynamic_config::Source ServiceBase::GetConfigSource() const {
  return config_storage_.GetSource();
}

void ServiceBase::AddServerMiddleware(
    std::shared_ptr<server::MiddlewareBase> middleware) {
  UINVARIANT(adding_middlewares_allowed_,
             "Adding server middlewares after the first RegisterService call "
             "is disallowed");
  server_middlewares_.push_back(std::move(middleware));
}

void ServiceBase::AddClientMiddleware(
    std::shared_ptr<const client::MiddlewareFactoryBase> middleware_factory) {
  UINVARIANT(adding_middlewares_allowed_,
             "Adding client middlewares after the StartServer call "
             "is disallowed");
  middleware_factories_.push_back(std::move(middleware_factory));
}

}  // namespace ugrpc::tests

USERVER_NAMESPACE_END
