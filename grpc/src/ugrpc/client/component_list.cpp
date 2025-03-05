#include <userver/ugrpc/client/component_list.hpp>

#include <userver/ugrpc/client/common_component.hpp>
#include <userver/ugrpc/client/middlewares/baggage/component.hpp>
#include <userver/ugrpc/client/middlewares/deadline_propagation/component.hpp>
#include <userver/ugrpc/client/middlewares/log/component.hpp>
#include <userver/ugrpc/middlewares/pipeline.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client {

components::ComponentList MinimalComponentList() {
    return components::ComponentList()
        .Append<CommonComponent>()
        .Append<MiddlewarePipelineComponent>()
        .Append<middlewares::deadline_propagation::Component>()
        .Append<middlewares::log::Component>();
}

components::ComponentList DefaultComponentList() {
    return components::ComponentList()
        .AppendComponentList(MinimalComponentList())
        .Append<middlewares::baggage::Component>();
}

}  // namespace ugrpc::client

USERVER_NAMESPACE_END
