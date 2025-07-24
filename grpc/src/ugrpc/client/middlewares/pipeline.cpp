#include <userver/ugrpc/client/middlewares/pipeline.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client {

MiddlewarePipelineComponent::MiddlewarePipelineComponent(
    const components::ComponentConfig& config,
    const components::ComponentContext& context
)
    : USERVER_NAMESPACE::middlewares::impl::AnyMiddlewarePipelineComponent(
          config,
          context,
          /*built-in middlewares=*/
          {{
              {"grpc-client-logging", {}},
              {"grpc-client-baggage", {}},
              {"grpc-client-deadline-propagation", {}},
              {"grpc-client-headers-propagator", {}},
          }}
      ) {}

}  // namespace ugrpc::client

USERVER_NAMESPACE_END
