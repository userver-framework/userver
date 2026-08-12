#pragma once

#include <health/v1/health_service.usrv.pb.hpp>

#include <userver/components/state.hpp>

namespace grpc::health::v1 {

class HealthHandler final : public HealthBase {
public:
    explicit HealthHandler(const USERVER_NAMESPACE::components::ComponentContext& context);

    CheckResult Check(USERVER_NAMESPACE::ugrpc::server::CallContext& context, HealthCheckRequest&& request) override;

private:
    const USERVER_NAMESPACE::components::State components_;
};

}  // namespace grpc::health::v1
