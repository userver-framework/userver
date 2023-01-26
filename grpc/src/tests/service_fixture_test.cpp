#include <tests/service_fixture_test.hpp>

#include <fmt/format.h>

#include <userver/engine/task/task.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

ugrpc::server::ServerConfig MakeServerConfig() {
  ugrpc::server::ServerConfig config;
  config.port = 0;
  return config;
}

}  // namespace

GrpcServiceFixture::GrpcServiceFixture()
    : server_(MakeServerConfig(), statistics_storage_) {}

GrpcServiceFixture::~GrpcServiceFixture() = default;

void GrpcServiceFixture::RegisterService(ugrpc::server::ServiceBase& service) {
  server_.AddService(service, engine::current_task::GetTaskProcessor());
}

void GrpcServiceFixture::StartServer(
    ugrpc::client::ClientFactoryConfig&& client_factory_config) {
  server_.Start();
  endpoint_ = fmt::format("[::1]:{}", server_.GetPort());
  client_factory_.emplace(std::move(client_factory_config),
                          engine::current_task::GetTaskProcessor(),
                          server_.GetCompletionQueue(), statistics_storage_);
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

ugrpc::server::Server& GrpcServiceFixture::GetServer() noexcept {
  return server_;
}

USERVER_NAMESPACE_END
