#include <tests/service_fixture_test.hpp>

#include <fmt/format.h>

#include <ugrpc/client/middlewares/deadline_propagation/middleware.hpp>
#include <ugrpc/client/middlewares/log/middleware.hpp>
#include <ugrpc/server/middlewares/deadline_propagation/middleware.hpp>
#include <ugrpc/server/middlewares/log/middleware.hpp>
#include <userver/dynamic_config/test_helpers.hpp>
#include <userver/engine/task/task.hpp>
#include <userver/logging/null_logger.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

ugrpc::server::ServerConfig MakeServerConfig() {
  ugrpc::server::ServerConfig config;
  config.port = 0;
  return config;
}

using ClientLogMiddlewareFactory =
    ugrpc::client::middlewares::log::MiddlewareFactory;
using ClientLogMiddlewareSettings =
    ugrpc::client::middlewares::log::Middleware::Settings;

using ClientDpMiddlewareFactory =
    ugrpc::client::middlewares::deadline_propagation::MiddlewareFactory;

using ServerLogMiddleware = ugrpc::server::middlewares::log::Middleware;
using ServerLogMiddlewareSettings =
    ugrpc::server::middlewares::log::Middleware::Settings;

using ServerDpMiddleware =
    ugrpc::server::middlewares::deadline_propagation::Middleware;

}  // namespace

GrpcServiceFixture::GrpcServiceFixture()
    : config_storage_(dynamic_config::MakeDefaultStorage({})),
      server_(MakeServerConfig(), statistics_storage_,
              logging::MakeNullLogger(), config_storage_.GetSource()),
      middleware_factories_({std::make_shared<ClientLogMiddlewareFactory>(
                                 ClientLogMiddlewareSettings{}),
                             std::make_shared<ClientDpMiddlewareFactory>()}),
      server_middlewares_(
          {std::make_shared<ServerLogMiddleware>(ServerLogMiddlewareSettings{}),
           std::make_shared<ServerDpMiddleware>()}),
      testsuite_({}, false) {}

GrpcServiceFixture::~GrpcServiceFixture() = default;

void GrpcServiceFixture::RegisterService(ugrpc::server::ServiceBase& service) {
  server_.AddService(service, engine::current_task::GetTaskProcessor(),
                     server_middlewares_);
}

void GrpcServiceFixture::StartServer(
    ugrpc::client::ClientFactoryConfig&& client_factory_config) {
  server_.Start();
  endpoint_ = fmt::format("[::1]:{}", server_.GetPort());
  client_factory_.emplace(std::move(client_factory_config),
                          engine::current_task::GetTaskProcessor(),
                          middleware_factories_, server_.GetCompletionQueue(),
                          statistics_storage_, testsuite_,
                          config_storage_.GetSource());
}

void GrpcServiceFixture::StopServer() noexcept {
  client_factory_.reset();
  endpoint_.reset();
  server_.Stop();
}

utils::statistics::Snapshot GrpcServiceFixture::GetStatistics(
    std::string prefix, std::vector<utils::statistics::Label> require_labels) {
  return utils::statistics::Snapshot{statistics_storage_, std::move(prefix),
                                     std::move(require_labels)};
}

void GrpcServiceFixture::ExtendDynamicConfig(
    const std::vector<dynamic_config::KeyValue>& overrides) {
  config_storage_.Extend(overrides);
}

ugrpc::server::Server& GrpcServiceFixture::GetServer() noexcept {
  return server_;
}

ugrpc::client::MiddlewareFactories&
GrpcServiceFixture::GetMiddlewareFactories() {
  return middleware_factories_;
}

ugrpc::server::Middlewares& GrpcServiceFixture::GetServerMiddlewares() {
  return server_middlewares_;
}

dynamic_config::Source GrpcServiceFixture::GetConfigSource() const {
  return config_storage_.GetSource();
}

USERVER_NAMESPACE_END
