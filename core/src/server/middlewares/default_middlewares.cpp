#include <userver/server/middlewares/default_middlewares.hpp>

#include <userver/components/component_list.hpp>

#include <server/middlewares/auth.hpp>
#include <server/middlewares/baggage.hpp>
#include <server/middlewares/deadline_propagation.hpp>
#include <server/middlewares/decompression.hpp>
#include <server/middlewares/exceptions_handling.hpp>
#include <server/middlewares/handler_adapter.hpp>
#include <server/middlewares/handler_metrics.hpp>
#include <server/middlewares/rate_limit.hpp>
#include <server/middlewares/tracing.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::middlewares {

components::ComponentList DefaultMiddlewaresList() {
  return components::ComponentList{}
      .Append<HandlerMetricsFactory>()
      .Append<TracingFactory>()
      .Append<BaggageFactory>()
      .Append<HandlerAdapterFactory>()
      .Append<RateLimitFactory>()
      .Append<AuthFactory>()
      .Append<DeadlinePropagationFactory>()
      .Append<DecompressionFactory>()
      .Append<SetAcceptEncodingFactory>()
      .Append<ExceptionsHandlingFactory>()
      .Append<UnknownExceptionsHandlingFactory>();
}

}  // namespace server::middlewares

USERVER_NAMESPACE_END
