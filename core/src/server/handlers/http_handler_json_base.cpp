#include <userver/server/handlers/http_handler_json_base.hpp>

#include <userver/formats/json/exception.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/http/content_type.hpp>
#include <userver/tracing/span.hpp>

#include <userver/server/handlers/exceptions.hpp>
#include <userver/server/handlers/json_error_builder.hpp>
#include <userver/server/handlers/legacy_json_error_builder.hpp>
#include <userver/server/http/http_error.hpp>
#include <userver/server/http/http_status.hpp>
#include <userver/yaml_config/schema.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::handlers {

namespace {

const std::string kRequestDataName = "__request_json";
const std::string kResponseDataName = "__response_json";
const std::string kSerializeJson = "serialize_json";

}  // namespace

HttpHandlerJsonBase::HttpHandlerJsonBase(
    const components::ComponentConfig& config,
    const components::ComponentContext& component_context, bool is_monitor)
    : HttpHandlerBase(config, component_context, is_monitor) {}

std::string HttpHandlerJsonBase::HandleRequestThrow(
    const http::HttpRequest& request, request::RequestContext& context) const {
  const auto& request_json =
      context.GetData<const formats::json::Value&>(kRequestDataName);

  auto& response = request.GetHttpResponse();
  response.SetContentType(
      USERVER_NAMESPACE::http::content_type::kApplicationJson);

  const auto& response_json = context.SetData<const formats::json::Value>(
      kResponseDataName,
      HandleRequestJsonThrow(request, request_json, context));

  const auto scope_time =
      tracing::Span::CurrentSpan().CreateScopeTime(kSerializeJson);
  return formats::json::ToString(response_json);
}

const formats::json::Value* HttpHandlerJsonBase::GetRequestJson(
    const request::RequestContext& context) {
  return context.GetDataOptional<const formats::json::Value>(kRequestDataName);
}

const formats::json::Value* HttpHandlerJsonBase::GetResponseJson(
    const request::RequestContext& context) {
  return context.GetDataOptional<const formats::json::Value>(kResponseDataName);
}

FormattedErrorData HttpHandlerJsonBase::GetFormattedExternalErrorBody(
    const CustomHandlerException& exc) const {
  if (exc.GetServiceCode().empty()) {
    // Legacy format has no "service codes", only HTTP codes.
    return {LegacyJsonErrorBuilder(exc).GetExternalBody(),
            LegacyJsonErrorBuilder::GetContentType()};
  }
  return {JsonErrorBuilder(exc).GetExternalBody(),
          JsonErrorBuilder::GetContentType()};
}

void HttpHandlerJsonBase::ParseRequestData(
    const http::HttpRequest& request, request::RequestContext& context) const {
  formats::json::Value request_json;
  try {
    if (!request.RequestBody().empty())
      request_json = formats::json::FromString(request.RequestBody());
  } catch (const formats::json::Exception& e) {
    throw RequestParseError(
        InternalMessage{"Invalid JSON body"},
        ExternalBody{std::string("Invalid JSON body: ") + e.what()});
  }

  context.SetData<const formats::json::Value>(kRequestDataName, request_json);
}

yaml_config::Schema HttpHandlerJsonBase::GetStaticConfigSchema() {
  auto schema = HttpHandlerBase::GetStaticConfigSchema();
  schema.UpdateDescription("HTTP handler JSON base config");
  return schema;
}

}  // namespace server::handlers

USERVER_NAMESPACE_END
