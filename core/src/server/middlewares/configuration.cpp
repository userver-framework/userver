#include <userver/server/middlewares/configuration.hpp>

#include <userver/components/component_config.hpp>
#include <userver/components/component_list.hpp>
#include <userver/server/middlewares/builtin.hpp>
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
      std::string{builtin::kHandlerMetrics},
      // Tracing should go before UnknownExceptionsHandlingMiddleware because it
      // adds some headers, which otherwise might be cleared
      std::string{builtin::kTracing},
      // Ditto
      std::string{builtin::kSetAcceptEncoding},

      // Every exception caught here is transformed into Http500 without
      // context.
      // All middlewares except for the most obscure ones should go below.
      std::string{builtin::kUnknownExceptionsHandling},

      // Should be self-explanatory
      std::string{builtin::kRateLimit},
      std::string{builtin::kBaggage},
      std::string{builtin::kAuth},
      std::string{builtin::kDecompression},

      // Transforms CustomHandlerException into response as specified by the
      // exception, transforms std::exception into Http500 without context.
      // Middlewares that throw CustomHandlerException on error should go below.
      // Middlewares that call HttpHandlerBase::HandleCustomHandlerException or
      // fill the response manually on error (which is faster) should go above.
      std::string{builtin::kExceptionsHandling},

      // DeadlinePropagation should go after ExceptionsHandlingMiddleware
      // if the request threw an std::exception and was canceled by deadline
      // propagation,
      // then it must be handled differently.
      std::string{builtin::kDeadlinePropagation},
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
    : components::ComponentBase{config, context},
      middlewares_to_append_{config["append"].As<MiddlewaresList>({})} {}

const MiddlewaresList& PipelineBuilder::GetMiddlewaresToAppend() const {
  return middlewares_to_append_;
}

yaml_config::Schema PipelineBuilder::GetStaticConfigSchema() {
  return yaml_config::MergeSchemas<components::ComponentBase>(R"(
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
    : components::ComponentBase{config, context} {}

}  // namespace server::middlewares

USERVER_NAMESPACE_END
