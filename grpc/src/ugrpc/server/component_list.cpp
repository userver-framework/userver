#include <userver/ugrpc/server/component_list.hpp>

#include <userver/components/minimal_component_list.hpp>
#include <userver/components/minimal_server_component_list.hpp>

#include <userver/ugrpc/middlewares/pipeline.hpp>
#include <userver/ugrpc/server/middlewares/baggage/component.hpp>
#include <userver/ugrpc/server/middlewares/congestion_control/component.hpp>
#include <userver/ugrpc/server/middlewares/deadline_propagation/component.hpp>
#include <userver/ugrpc/server/middlewares/headers_propagator/component.hpp>
#include <userver/ugrpc/server/middlewares/log/component.hpp>
#include <userver/ugrpc/server/server_component.hpp>
#include <userver/ugrpc/server/service_component_base.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server {

components::ComponentList MinimalComponentList() {
    return components::ComponentList()
        .Append<ServerComponent>()
        .Append<ugrpc::middlewares::MiddlewarePipelineComponent>(kMiddlewarePipelineName)
        .Append<middlewares::congestion_control::Component>()
        .Append<middlewares::deadline_propagation::Component>()
        .Append<middlewares::log::Component>();
}

components::ComponentList DefaultComponentList() {
    return components::ComponentList()
        .AppendComponentList(MinimalComponentList())
        .Append<middlewares::baggage::Component>()
        .Append<middlewares::headers_propagator::Component>();
}

}  // namespace ugrpc::server

USERVER_NAMESPACE_END
