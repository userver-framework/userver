#include <userver/ugrpc/server/health/health.hpp>

#include <userver/components/component_context.hpp>
#include <userver/logging/log.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server {

namespace {

bool IsServing(const components::State& components_state) {
    if (components_state.IsAnyComponentInFatalState()) {
        return false;
    }

    const auto lifetime_stage = components_state.GetServiceLifetimeStage();
    if (lifetime_stage != components::ServiceLifetimeStage::kRunning) {
        LOG_WARNING() << "Service is not ready for requests (stage=" << ToString(lifetime_stage)
                      << "), returning NOT_SERVING from Health/Check";
        return false;
    }

    return true;
}

}  // namespace

HealthHandler::HealthHandler(const components::ComponentContext& context) : components_(context) {}

HealthHandler::CheckResult
HealthHandler::Check([[maybe_unused]] CallContext& context, ::grpc::health::v1::HealthCheckRequest&&) {
    ::grpc::health::v1::HealthCheckResponse response;
    if (IsServing(components_)) {
        response.set_status(::grpc::health::v1::HealthCheckResponse::SERVING);
    } else {
        response.set_status(::grpc::health::v1::HealthCheckResponse::NOT_SERVING);
    }
    return response;
}

}  // namespace ugrpc::server

USERVER_NAMESPACE_END
