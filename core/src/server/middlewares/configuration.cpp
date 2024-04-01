#include <userver/server/middlewares/configuration.hpp>

#include <userver/components/component_config.hpp>
#include <userver/components/component_list.hpp>
#include <userver/testsuite/middlewares.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

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

/// [Middlewares sample - default pipeline]
MiddlewaresList DefaultPipeline() {
  return {
      // Metrics should go before everything else, basically.
      std::string{HandlerMetrics::kName},
      // Tracing should go before UnknownExceptionsHandlingMiddleware because it
      // adds some headers, which otherwise might be cleared
      std::string{Tracing::kName},
      // Ditto
      std::string{SetAcceptEncoding::kName},

      // Every exception caught here is transformed into Http500 without context
      std::string{UnknownExceptionsHandling::kName},

      // Should be self-explanatory
      std::string{RateLimit::kName},
      std::string{DeadlinePropagation::kName},
      std::string{Baggage::kName},
      std::string{Auth::kName},
      std::string{Decompression::kName},

      // Transforms CustomHandlerException into response as specified by the
      // exception, transforms std::exception into Http500 without context
      std::string{ExceptionsHandling::kName},
  };
}
/// [Middlewares sample - default pipeline]

components::ComponentList DefaultMiddlewareComponents() {
  return MinimalMiddlewareComponents()
      .Append<HandlerMetricsFactory>()
      .Append<TracingFactory>()
      .Append<BaggageFactory>()
      .Append<RateLimitFactory>()
      .Append<AuthFactory>()
      .Append<DeadlinePropagationFactory>()
      .Append<DecompressionFactory>()
      .Append<SetAcceptEncodingFactory>()
      .Append<ExceptionsHandlingFactory>()
      .Append<UnknownExceptionsHandlingFactory>()
      .Append<testsuite::ExceptionsHandlingMiddlewareFactory>();
}

components::ComponentList MinimalMiddlewareComponents() {
  return components::ComponentList{}
      .Append<PipelineBuilder>()
      .Append<HandlerPipelineBuilder>()
      .Append<HandlerAdapterFactory>();
}

PipelineBuilder::PipelineBuilder(const components::ComponentConfig& config,
                                 const components::ComponentContext& context)
    : components::LoggableComponentBase{config, context},
      middlewares_to_append_{config["append"].As<MiddlewaresList>({})} {}

const MiddlewaresList& PipelineBuilder::GetMiddlewaresToAppend() const {
  return middlewares_to_append_;
}

yaml_config::Schema PipelineBuilder::GetStaticConfigSchema() {
  return yaml_config::MergeSchemas<components::LoggableComponentBase>(R"(
type: object
description: Base class for a component to configure server middlewares
additionalProperties: false
properties:
    append:
        type: array
        items:
            type: string
            description: name of a middleware to append
        description: list of middlewares to append by default
        defaultDescription: an empty list
)");
}

HandlerPipelineBuilder::HandlerPipelineBuilder(
    const components::ComponentConfig& config,
    const components::ComponentContext& context)
    : components::LoggableComponentBase{config, context} {}

}  // namespace server::middlewares

USERVER_NAMESPACE_END
