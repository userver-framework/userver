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

GrpcServiceFixture::GrpcServiceFixture() : server_(MakeServerConfig()) {}

GrpcServiceFixture::~GrpcServiceFixture() = default;

void GrpcServiceFixture::RegisterService(ugrpc::server::ServiceBase& service) {
  server_.AddService(service, engine::current_task::GetTaskProcessor());
}

void GrpcServiceFixture::StartServer() {
  server_.Start();
  endpoint_ = fmt::format("[::1]:{}", server_.GetPort());
  client_factory_.emplace(engine::current_task::GetTaskProcessor(),
                          server_.GetCompletionQueue());
}

void GrpcServiceFixture::StopServer() noexcept {
  client_factory_.reset();
  endpoint_.reset();
  server_.Stop();
}

USERVER_NAMESPACE_END
