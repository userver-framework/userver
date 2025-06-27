#include <userver/ugrpc/server/health/component.hpp>

#include <ugrpc/server/health/health.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server {

HealthComponent::HealthComponent(const components::ComponentConfig& config, const components::ComponentContext& context)
    : ServiceComponentBase{config, context}, service_{context} {
    RegisterService(*service_);
}

HealthComponent::~HealthComponent() = default;

}  // namespace ugrpc::server

USERVER_NAMESPACE_END
