#include <userver/ugrpc/server/middlewares/pipeline.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server {

MiddlewarePipelineComponent::MiddlewarePipelineComponent(
    const components::ComponentConfig& config,
    const components::ComponentContext& context
)
    : USERVER_NAMESPACE::middlewares::impl::AnyMiddlewarePipelineComponent(
          config,
          context,
          {/*built-in middlewares=*/{
              {"grpc-server-access-log", {}},
              {"grpc-server-logging", {}},
              {"grpc-server-baggage", {}},
              {"grpc-server-congestion-control", {}},
              {"grpc-server-deadline-propagation", {}},
              {"grpc-server-headers-propagator", {}},
          }}
      ) {}

}  // namespace ugrpc::server

USERVER_NAMESPACE_END
