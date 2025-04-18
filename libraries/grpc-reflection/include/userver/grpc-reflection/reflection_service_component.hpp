#pragma once

/// @file userver/grpc-reflection/reflection_service_component.hpp
/// @brief @copybrief grpc_reflection::ReflectionServiceComponent

#include <string_view>

#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/ugrpc/server/server.hpp>
#include <userver/ugrpc/server/service_component_base.hpp>

USERVER_NAMESPACE_BEGIN

/// @brief Top namespace for grpc-reflection library
///
/// For more information see @ref scripts/docs/en/userver/libraries/grpc-reflection.md.
namespace grpc_reflection {

class ProtoServerReflection;

class ReflectionServiceComponent final : public ugrpc::server::ServiceComponentBase {
public:
    static constexpr std::string_view kName = "grpc-reflection-service";

    ReflectionServiceComponent(const components::ComponentConfig& config, const components::ComponentContext& context);

    ~ReflectionServiceComponent() override;

private:
    void OnAllComponentsLoaded() override;

    components::ComponentHealth GetComponentHealth() const override;

    void AddService(std::string_view service_name);

    std::unique_ptr<grpc_reflection::ProtoServerReflection> service_;
    ugrpc::server::Server& ugrpc_server_;
    std::atomic<bool> ready_{false};
};

}  // namespace grpc_reflection

USERVER_NAMESPACE_END
