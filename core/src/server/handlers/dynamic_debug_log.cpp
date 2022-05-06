#include <userver/server/handlers/dynamic_debug_log.hpp>

#include <boost/algorithm/string/split.hpp>

#include <logging/dynamic_debug.hpp>
#include <userver/components/component.hpp>
#include <userver/fs/read.hpp>
#include <userver/logging/level.hpp>
#include <userver/logging/log.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::handlers {

DynamicDebugLog::DynamicDebugLog(const components::ComponentConfig& config,
                                 const components::ComponentContext& context)
    : HttpHandlerBase(config, context, /*is_monitor = */ true) {
  logging::InitDynamicDebugLog();
}

std::string DynamicDebugLog::HandleRequestThrow(
    const http::HttpRequest& request, request::RequestContext& context) const {
  switch (request.GetMethod()) {
    case http::HttpMethod::kGet:
      return ProcessGet(request, context);
    case http::HttpMethod::kPost:
      return ProcessPost(request, context);
    default:
      throw std::runtime_error("unsupported method: " + request.GetMethodStr());
  }
}

std::string DynamicDebugLog::ProcessPost(const http::HttpRequest& request,
                                         request::RequestContext&) {
  const auto& location = request.GetArg("location");
  const auto& body = request.RequestBody();

  logging::SetDynamicDebugLog(location, !body.empty());
  return "OK\n";
}

std::string DynamicDebugLog::ProcessGet(const http::HttpRequest& request,
                                        request::RequestContext&) {
  const auto& location = request.GetArg("location");

  if (!location.empty()) {
    auto enabled = logging::DynamicDebugShouldLogRelative(location);
    return std::to_string(enabled) + "\n";
  } else {
    auto locations = logging::GetDynamicDebugLocations();
    std::string result;
    for (const auto& location : locations) {
      auto enabled = logging::DynamicDebugShouldLogRelative(location);
      result += fmt::format("{}\t{}\n", location, enabled ? 1 : 0);
    }
    return result;
  }
}

yaml_config::Schema DynamicDebugLog::GetStaticConfigSchema() {
  return yaml_config::MergeSchemas<HttpHandlerBase>(R"(
type: object
description: Handler to show/hide logs at the specific file:line
additionalProperties: false
properties: {}
)");
}

}  // namespace server::handlers

USERVER_NAMESPACE_END
