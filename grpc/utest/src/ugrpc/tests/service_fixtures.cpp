#include <userver/ugrpc/tests/service_fixtures.hpp>

#include <fmt/format.h>

#include <ugrpc/client/middlewares/deadline_propagation/middleware.hpp>
#include <ugrpc/client/middlewares/log/middleware.hpp>
#include <ugrpc/server/middlewares/deadline_propagation/middleware.hpp>
#include <ugrpc/server/middlewares/log/middleware.hpp>
#include <userver/dynamic_config/test_helpers.hpp>
#include <userver/engine/task/task.hpp>
#include <userver/logging/null_logger.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::tests {

namespace {

server::ServerConfig MakeServerConfig() {
  server::ServerConfig config;
  config.port = 0;
  return config;
}

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

ServiceFixtureBase::ServiceFixtureBase(dynamic_config::StorageMock&& dynconf)
    : config_storage_(std::move(dynconf)),
      server_(MakeServerConfig(), statistics_storage_,
              config_storage_.GetSource()),
      server_middlewares_(
          {std::make_shared<ServerLogMiddleware>(ServerLogMiddlewareSettings{}),
           std::make_shared<ServerDpMiddleware>()}),
      middleware_factories_({std::make_shared<ClientLogMiddlewareFactory>(
                                 ClientLogMiddlewareSettings{}),
                             std::make_shared<ClientDpMiddlewareFactory>()}),
      testsuite_({}, false) {}

ServiceFixtureBase::~ServiceFixtureBase() = default;

void ServiceFixtureBase::RegisterService(server::ServiceBase& service) {
  adding_middlewares_allowed_ = false;
  server_.AddService(service, server::ServiceConfig{
                                  engine::current_task::GetTaskProcessor(),
                                  server_middlewares_,
                              });
}

void ServiceFixtureBase::StartServer(
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

void ServiceFixtureBase::StopServer() noexcept {
  client_factory_.reset();
  endpoint_.reset();
  server_.Stop();
}

utils::statistics::Snapshot ServiceFixtureBase::GetStatistics(
    std::string prefix, std::vector<utils::statistics::Label> require_labels) {
  return utils::statistics::Snapshot{statistics_storage_, std::move(prefix),
                                     std::move(require_labels)};
}

void ServiceFixtureBase::ExtendDynamicConfig(
    const std::vector<dynamic_config::KeyValue>& overrides) {
  config_storage_.Extend(overrides);
}

server::Server& ServiceFixtureBase::GetServer() noexcept { return server_; }

dynamic_config::Source ServiceFixtureBase::GetConfigSource() const {
  return config_storage_.GetSource();
}

void ServiceFixtureBase::AddServerMiddleware(
    std::shared_ptr<server::MiddlewareBase> middleware) {
  UINVARIANT(adding_middlewares_allowed_,
             "Adding server middlewares after the first RegisterService call "
             "is disallowed");
  server_middlewares_.push_back(std::move(middleware));
}

void ServiceFixtureBase::AddClientMiddleware(
    std::shared_ptr<const client::MiddlewareFactoryBase> middleware_factory) {
  UINVARIANT(adding_middlewares_allowed_,
             "Adding client middlewares after the StartServer call "
             "is disallowed");
  middleware_factories_.push_back(std::move(middleware_factory));
}

}  // namespace ugrpc::tests

USERVER_NAMESPACE_END
