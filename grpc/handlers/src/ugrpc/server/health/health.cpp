#include <userver/ugrpc/server/health/health.hpp>

#include <userver/components/component_context.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server {

HealthHandler::HealthHandler(const components::ComponentContext& context)
    : context_(context) {}

void HealthHandler::Check(CheckCall& call,
                          ::grpc::health::v1::HealthCheckRequest&&) {
  ::grpc::health::v1::HealthCheckResponse response;
  if (context_.IsAnyComponentInFatalState())
    response.set_status(::grpc::health::v1::HealthCheckResponse::NOT_SERVING);
  else
    response.set_status(::grpc::health::v1::HealthCheckResponse::SERVING);
  call.Finish(response);
}

}  // namespace ugrpc::server

USERVER_NAMESPACE_END
