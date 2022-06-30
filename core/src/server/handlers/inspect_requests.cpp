#include <userver/server/handlers/inspect_requests.hpp>

#include <cctz/time_zone.h>

#include <server/http/http_request_impl.hpp>
#include <server/requests_view.hpp>
#include <userver/server/component.hpp>
#include <userver/yaml_config/schema.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::handlers {

InspectRequests::InspectRequests(
    const components::ComponentConfig& config,
    const components::ComponentContext& component_context)
    : HttpHandlerJsonBase(config, component_context, /* is_monitor = */ true),
      view_(component_context.FindComponent<components::Server>()
                .GetServer()
                .GetRequestsView()) {}

formats::json::ValueBuilder FormatHeadersAsJson(
    const http::HttpRequestImpl& request) {
  formats::json::ValueBuilder result(formats::json::Type::kObject);
  for (const auto& name : request.GetHeaderNames())
    result[name] = request.GetHeader(name);

  return result;
}

formats::json::ValueBuilder FormatArgsAsJson(
    const http::HttpRequestImpl& request) {
  formats::json::ValueBuilder result(formats::json::Type::kObject);
  for (const auto& name : request.ArgNames()) {
    formats::json::ValueBuilder values_json(formats::json::Type::kArray);
    for (const auto& value : request.GetArgVector(name))
      values_json.PushBack(value);

    result[name] = values_json;
  }
  return result;
}

formats::json::ValueBuilder FormatCookiesAsJson(
    const http::HttpRequestImpl& request) {
  formats::json::ValueBuilder result(formats::json::Type::kObject);
  for (const auto& name : request.GetCookieNames())
    result[name] = request.GetCookie(name);

  return result;
}

formats::json::ValueBuilder FormatHandlingDuration(
    const http::HttpRequestImpl& request) {
  auto start_time = request.StartTime();
  auto now = std::chrono::steady_clock::now();
  auto duration = now - start_time;
  auto duration_ms =
      std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
  return duration_ms;
}

formats::json::ValueBuilder FormatStartHandlingTimestamp(
    const http::HttpRequestImpl& request) {
  auto start_time = request.StartTime();
  auto now = std::chrono::steady_clock::now();
  auto duration = now - start_time;
  auto now_system = std::chrono::system_clock::now();
  auto ts = now_system - duration;

  static const auto kTz = cctz::utc_time_zone();
  static const std::string kFormatString = "%a, %d %b %Y %H:%M:%S %Z";
  return cctz::format(kFormatString, ts, kTz);
}

formats::json::Value InspectRequests::HandleRequestJsonThrow(
    const http::HttpRequest& request, const formats::json::Value&,
    request::RequestContext&) const {
  const bool with_body = !request.GetArg("body").empty();

  formats::json::ValueBuilder result(formats::json::Type::kArray);
  auto requests = view_.GetAllRequests();
  LOG_INFO() << "Got " << requests.size() << " requests";
  for (const auto& base_request : requests) {
    /* TODO: change if we support non-HTTP requests */
    auto* request = dynamic_cast<http::HttpRequestImpl*>(base_request.get());

    formats::json::ValueBuilder request_json(formats::json::Type::kObject);
    if (with_body) request_json["request-body"] = request->RequestBody();
    request_json["method"] = request->GetMethodStr();
    request_json["url"] = request->GetUrl();
    request_json["http_version"] = std::to_string(request->GetHttpMajor()) +
                                   "." +
                                   std::to_string(request->GetHttpMinor());
    request_json["request_path"] = request->GetRequestPath();
    request_json["handling-duration-ms"] = FormatHandlingDuration(*request);
    request_json["start-handling-timestamp"] =
        FormatStartHandlingTimestamp(*request);
    request_json["host"] = request->GetHost();

    request_json["args"] = FormatArgsAsJson(*request);
    request_json["headers"] = FormatHeadersAsJson(*request);

    result.PushBack(std::move(request_json));
  }

  return result.ExtractValue();
}
yaml_config::Schema InspectRequests::GetStaticConfigSchema() {
  auto schema = HttpHandlerBase::GetStaticConfigSchema();
  schema.UpdateDescription("handler-inspect-requests config");
  return schema;
}

}  // namespace server::handlers

USERVER_NAMESPACE_END
