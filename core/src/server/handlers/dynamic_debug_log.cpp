#include <userver/server/handlers/dynamic_debug_log.hpp>

#include <boost/algorithm/string/split.hpp>

#include <logging/dynamic_debug.hpp>
#include <logging/split_location.hpp>
#include <userver/components/component.hpp>
#include <userver/fs/read.hpp>
#include <userver/logging/level.hpp>
#include <userver/logging/log.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::handlers {

namespace {

std::string StateToString(logging::EntryState state) {
  if (state.force_enabled_level != logging::Level::kNone) {
    return "1";
  } else if (state.force_disabled_level_plus_one != logging::Level::kTrace) {
    return "-1";
  } else {
    return "0";
  }
}

std::string ProcessGet(const http::HttpRequest& request,
                       request::RequestContext&) {
  std::string_view location = request.GetArg("location");

  const auto& locations = logging::GetDynamicDebugLocations();
  if (!location.empty()) {
    auto [path, line] = logging::SplitLocation(location);
    auto it = locations.find({path.c_str(), line});
    if (it == locations.end()) {
      request.SetResponseStatus(server::http::HttpStatus::kNotFound);
      return "Location not found\n";
    }

    return StateToString(it->state.load()) + "\n";
  } else {
    std::string result;
    for (const auto& location : locations) {
      // TODO
      const auto enabled = StateToString(location.state.load());
      result +=
          fmt::format("{}:{}\t{}\n", location.path, location.line, enabled);
    }
    return result;
  }
}

std::string ProcessPut(const http::HttpRequest& request,
                       request::RequestContext&) {
  const auto& location = request.GetArg("location");
  auto [path, line] = logging::SplitLocation(location);

  const auto& body = request.RequestBody();
  logging::EntryState state;
  if (body.empty() || body == "1") {
    state.force_enabled_level = logging::Level::kTrace;
  } else if (body == "-1") {
    state.force_disabled_level_plus_one = logging::Level::kWarning;
  } else if (body == "0") {
    state = logging::EntryState{};
  } else {
    throw std::runtime_error("Bad body");
  }

  logging::AddDynamicDebugLog(path, line, state);
  return "OK\n";
}

std::string ProcessDelete(const http::HttpRequest& request,
                          request::RequestContext&) {
  const auto& location = request.GetArg("location");
  auto [path, line] = logging::SplitLocation(location);
  logging::RemoveDynamicDebugLog(path, line);
  return "OK\n";
}

}  // namespace

DynamicDebugLog::DynamicDebugLog(const components::ComponentConfig& config,
                                 const components::ComponentContext& context)
    : HttpHandlerBase(config, context, /*is_monitor = */ true) {}

std::string DynamicDebugLog::HandleRequestThrow(
    const http::HttpRequest& request, request::RequestContext& context) const {
  switch (request.GetMethod()) {
    case http::HttpMethod::kGet:
      return ProcessGet(request, context);
    case http::HttpMethod::kPut:
      return ProcessPut(request, context);
    case http::HttpMethod::kDelete:
      return ProcessDelete(request, context);
    default:
      throw std::runtime_error("unsupported method: " + request.GetMethodStr());
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
