#include <userver/ugrpc/server/component_list.hpp>

#include <userver/components/minimal_component_list.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/congestion_control/component.hpp>

#include <userver/ugrpc/server/middlewares/baggage/component.hpp>
#include <userver/ugrpc/server/middlewares/congestion_control/component.hpp>
#include <userver/ugrpc/server/middlewares/deadline_propagation/component.hpp>
#include <userver/ugrpc/server/middlewares/field_mask/component.hpp>
#include <userver/ugrpc/server/middlewares/headers_propagator/component.hpp>
#include <userver/ugrpc/server/middlewares/log/component.hpp>
#include <userver/ugrpc/server/middlewares/pipeline.hpp>
#include <userver/ugrpc/server/server_component.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server {

components::ComponentList DefaultComponentList() {
    return components::ComponentList()
        .Append<ServerComponent>()
        .Append<MiddlewarePipelineComponent>()
        .Append<middlewares::baggage::Component>()
        .Append<middlewares::congestion_control::Component>()
        .Append<middlewares::deadline_propagation::Component>()
        .Append<middlewares::field_mask::Component>()
        .Append<middlewares::headers_propagator::Component>()
        .Append<middlewares::log::Component>()
        .Append<USERVER_NAMESPACE::congestion_control::Component>();
    ;
}

components::ComponentList MinimalComponentList() {
    return components::ComponentList()
        .Append<ServerComponent>()
        .Append<MiddlewarePipelineComponent>()
        .Append<middlewares::baggage::Component>()
        .Append<middlewares::congestion_control::Component>()
        .Append<middlewares::deadline_propagation::Component>()
        .Append<middlewares::log::Component>()
        .Append<USERVER_NAMESPACE::congestion_control::Component>();
}

}  // namespace ugrpc::server

USERVER_NAMESPACE_END
