#include <userver/ugrpc/server/health/health.hpp>

#include <userver/components/component_context.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server {

HealthHandler::HealthHandler(const components::ComponentContext& context)
    : components_(context) {}

HealthHandler::CheckResult HealthHandler::Check(
    [[maybe_unused]] CallContext& context,
    ::grpc::health::v1::HealthCheckRequest&&) {
  ::grpc::health::v1::HealthCheckResponse response;
  if (components_.IsAnyComponentInFatalState()) {
    response.set_status(::grpc::health::v1::HealthCheckResponse::NOT_SERVING);
  } else {
    response.set_status(::grpc::health::v1::HealthCheckResponse::SERVING);
  }
  return response;
}

}  // namespace ugrpc::server

USERVER_NAMESPACE_END
