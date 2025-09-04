#include <userver/ugrpc/client/component_list.hpp>

#include <userver/ugrpc/client/common_component.hpp>
#include <userver/ugrpc/client/middlewares/baggage/component.hpp>
#include <userver/ugrpc/client/middlewares/deadline_propagation/component.hpp>
#include <userver/ugrpc/client/middlewares/headers_propagator/component.hpp>
#include <userver/ugrpc/client/middlewares/log/component.hpp>
#include <userver/ugrpc/client/middlewares/pipeline.hpp>
#include <userver/ugrpc/client/middlewares/testsuite/component.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client {

components::ComponentList MinimalComponentList() {
    return components::ComponentList()
        .Append<CommonComponent>()
        .Append<MiddlewarePipelineComponent>()
        .Append<middlewares::deadline_propagation::Component>()
        .Append<middlewares::log::Component>()
        .Append<middlewares::testsuite::Component>();
}

components::ComponentList DefaultComponentList() {
    return components::ComponentList()
        .AppendComponentList(MinimalComponentList())
        .Append<middlewares::headers_propagator::Component>()
        .Append<middlewares::baggage::Component>();
}

}  // namespace ugrpc::client

USERVER_NAMESPACE_END
