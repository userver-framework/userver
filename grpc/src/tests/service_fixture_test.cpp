#include <tests/service_fixture_test.hpp>

#include <fmt/format.h>

#include <userver/engine/task/task.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/utils/statistics/storage.hpp>

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

void GrpcServiceFixture::StartServer() {
  server_.Start();
  endpoint_ = fmt::format("[::1]:{}", server_.GetPort());
  client_factory_.emplace(ugrpc::client::ClientFactoryConfig{},
                          engine::current_task::GetTaskProcessor(),
                          server_.GetCompletionQueue(), statistics_storage_);
}

void GrpcServiceFixture::StopServer() noexcept {
  client_factory_.reset();
  endpoint_.reset();
  server_.Stop();
}

formats::json::Value GrpcServiceFixture::GetStatistics() {
  return statistics_storage_.GetAsJson(utils::statistics::StatisticsRequest{""})
      .ExtractValue();
}

ugrpc::server::Server& GrpcServiceFixture::GetServer() noexcept {
  return server_;
}

USERVER_NAMESPACE_END
