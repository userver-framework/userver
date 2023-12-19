#include <userver/server/handlers/ping.hpp>

#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/server/handlers/exceptions.hpp>
#include <userver/yaml_config/merge_schemas.hpp>
#include <userver/yaml_config/schema.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::handlers {

Ping::Ping(const components::ComponentConfig& config,
           const components::ComponentContext& component_context)
    : HttpHandlerBase(config, component_context),
      components_(component_context),
      awacs_weight_warmup_time_(config["warmup-time-secs"].As<int>(0)) {}

std::string Ping::HandleRequestThrow(
    const http::HttpRequest& request,
    request::RequestContext& /*context*/) const {
  if (components_.IsAnyComponentInFatalState()) {
    throw InternalServerError();
  }

  auto& response = request.GetHttpResponse();
  AppendWeightHeaders(response);

  return {};
}

void Ping::OnAllComponentsLoaded() {
  load_time_ = std::chrono::steady_clock::now();
}

void Ping::AppendWeightHeaders(http::HttpResponse& response) const {
  if (awacs_weight_warmup_time_.count()) {
    auto now = std::chrono::steady_clock::now();
    auto diff = now - load_time_;
    auto weight = 1000 * diff / awacs_weight_warmup_time_;
    if (weight > 1000) weight = 1000;

    response.SetHeader(std::string_view{"RS-Weight"},
                       std::to_string(weight / 1000.0));
  }
}

yaml_config::Schema Ping::GetStaticConfigSchema() {
  return yaml_config::MergeSchemas<HttpHandlerBase>(R"(
type: object
description: ping handler config
additionalProperties: false
properties:
    warmup-time-secs:
        type: integer
        description: time to server warmup, set to zero to disable the feature
        defaultDescription: 0
)");
}

}  // namespace server::handlers

USERVER_NAMESPACE_END
