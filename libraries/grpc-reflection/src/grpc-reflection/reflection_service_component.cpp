#include <userver/grpc-reflection/reflection_service_component.hpp>

#include <userver/logging/log.hpp>
#include <userver/ugrpc/server/server_component.hpp>
#include <userver/ugrpc/server/service_component_base.hpp>

#include <grpc-reflection/proto_server_reflection.hpp>

USERVER_NAMESPACE_BEGIN

namespace grpc_reflection {

ReflectionServiceComponent::ReflectionServiceComponent(
    const components::ComponentConfig& config,
    const components::ComponentContext& context
)
    : ugrpc::server::ServiceComponentBase(config, context),
      service_(std::make_unique<grpc_reflection::ProtoServerReflection>()),
      ugrpc_server_(context.FindComponent<ugrpc::server::ServerComponent>().GetServer()) {
    LOG_INFO() << "Preparing to register service";
    RegisterService(*service_);
    LOG_INFO() << "Service registered";
}

ReflectionServiceComponent::~ReflectionServiceComponent() = default;

void ReflectionServiceComponent::AddService(std::string_view service) { service_->AddService(service); }

void ReflectionServiceComponent::OnAllComponentsLoaded() {
    const auto& service_names = ugrpc_server_.GetServiceNames();
    service_->AddServiceList(service_names);
    ready_.store(true);
}

components::ComponentHealth ReflectionServiceComponent::GetComponentHealth() const {
    return ready_.load() ? components::ComponentHealth::kOk : components::ComponentHealth::kFatal;
}

}  // namespace grpc_reflection

USERVER_NAMESPACE_END
