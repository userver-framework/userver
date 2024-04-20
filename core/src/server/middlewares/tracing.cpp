#include <server/middlewares/tracing.hpp>

#include <server/handlers/http_server_settings.hpp>
#include <server/middlewares/misc.hpp>
#include <server/request/internal_request_context.hpp>

#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/http/common_headers.hpp>
#include <userver/logging/level_serialization.hpp>
#include <userver/server/handlers/handler_config.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/server/http/http_request.hpp>
#include <userver/server/request/request_context.hpp>
#include <userver/tracing/manager_component.hpp>
#include <userver/tracing/span_builder.hpp>
#include <userver/tracing/tags.hpp>
#include <userver/utils/fast_scope_guard.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::middlewares {

namespace {

const std::string kTracingTypeResponse = "response";
const std::string kTracingBody = "body";
const std::string kTracingUri = "uri";

std::string GetHeadersLogString(const http::HttpResponse& response) {
  formats::json::ValueBuilder json_headers(formats::json::Type::kObject);
  for (const auto& header_name : response.GetHeaderNames()) {
    json_headers[header_name] = response.GetHeader(header_name);
  }
  return formats::json::ToString(json_headers.ExtractValue());
}

void LogYandexHeaders(const http::HttpRequest& http_request) {
  if (logging::ShouldLog(logging::Level::kInfo)) {
    const auto& yandex_request_id =
        http_request.GetHeader(USERVER_NAMESPACE::http::headers::kXRequestId);
    const auto& yandex_backend_server = http_request.GetHeader(
        USERVER_NAMESPACE::http::headers::kXBackendServer);
    const auto& envoy_proxy = http_request.GetHeader(
        USERVER_NAMESPACE::http::headers::kXTaxiEnvoyProxyDstVhost);

    if (!yandex_request_id.empty() || !yandex_backend_server.empty() ||
        !envoy_proxy.empty()) {
      LOG_INFO() << fmt::format(
          "Yandex tracing headers {}={}, {}={}, {}={}",
          USERVER_NAMESPACE::http::headers::kXRequestId, yandex_request_id,
          USERVER_NAMESPACE::http::headers::kXBackendServer,
          yandex_backend_server,
          USERVER_NAMESPACE::http::headers::kXTaxiEnvoyProxyDstVhost,
          envoy_proxy);
    }
  }
}

}  // namespace

Tracing::Tracing(const tracing::TracingManagerBase& tracing_manager,
                 const handlers::HttpHandlerBase& handler)
    : tracing_manager_{tracing_manager},
      handler_{handler},
      log_level_{handler_.GetLogLevel()} {}

void Tracing::HandleRequest(http::HttpRequest& request,
                            request::RequestContext& context) const {
  const auto meta_type = misc::CutTrailingSlash(
      request.GetRequestPath(), handler_.GetConfig().url_trailing_slash);
  auto span = MakeSpan(request, meta_type);
  LogYandexHeaders(request);

  // This needs ConfigSnapshot, which is reset down the call chain in Next(),
  // so we prepare settings here
  const auto logging_settings = ParseLoggingSettings(context);
  const utils::FastScopeGuard guard{
      [this, &span, &logging_settings, &request, &context]() noexcept {
        try {
          FillResponseWithTracingContext(span, request.GetHttpResponse());
          EnrichLogs(span, logging_settings, request, context);
        } catch (const std::exception& ex) {
          // Something went really wrong if our tracing threw itself.
          LOG_ERROR() << "Failed to set tracing context for response: " << ex;
        } catch (...) {
          // Something went terribly wrong if our tracing threw non-std
          // exception itself.
          LOG_ERROR() << "Failed to set tracing context for response due to an "
                         "unknown exception (task cancellation?)";
        }
      }};

  Next(request, context);
}

tracing::Span Tracing::MakeSpan(const http::HttpRequest& http_request,
                                std::string_view meta_type) const {
  tracing::SpanBuilder span_builder(
      fmt::format("http/{}", handler_.HandlerName()));
  tracing_manager_.TryFillSpanBuilderFromRequest(http_request, span_builder);
  auto span = std::move(span_builder).Build();

  span.SetLocalLogLevel(log_level_);

  span.AddNonInheritableTag(tracing::kHttpMetaType, std::string{meta_type});
  span.AddNonInheritableTag(tracing::kType, kTracingTypeResponse);
  span.AddNonInheritableTag(tracing::kHttpMethod, http_request.GetMethodStr());

  return span;
}

void Tracing::FillResponseWithTracingContext(
    const tracing::Span& span, http::HttpResponse& response) const {
  if (handler_.GetConfig().set_tracing_headers) {
    tracing_manager_.FillResponseWithTracingContext(span, response);
  }
}

Tracing::LoggingSettings Tracing::ParseLoggingSettings(
    request::RequestContext& context) const {
  const auto& config_snapshot =
      context.GetInternalContext().GetConfigSnapshot();

  return {config_snapshot[handlers::kLogRequest],
          config_snapshot[handlers::kLogRequestHeaders]};
}

void Tracing::EnrichLogs(tracing::Span& span,
                         const LoggingSettings& logging_settings,
                         const http::HttpRequest& request,
                         request::RequestContext& context) const {
  try {
    auto& response = request.GetHttpResponse();

    const auto status_code = response.GetStatus();
    const auto& forced_log_level_opt =
        context.GetInternalContext().GetDPContext().GetForcedLogLevel();
    span.SetLogLevel(forced_log_level_opt.has_value()
                         ? *forced_log_level_opt
                         : handler_.GetLogLevelForResponseStatus(status_code));
    if (!span.ShouldLogDefault()) {
      return;
    }

    int response_code = static_cast<int>(status_code);
    span.AddTag(tracing::kHttpStatusCode, response_code);
    if (response_code >= 500) span.AddTag(tracing::kErrorFlag, true);

    if (logging_settings.need_log_response) {
      if (logging_settings.need_log_response_headers) {
        span.AddNonInheritableTag("response_headers",
                                  GetHeadersLogString(response));
      }
      span.AddNonInheritableTag(kTracingBody,
                                handler_.GetResponseDataForLoggingChecked(
                                    request, context, response.GetData()));
    }
    span.AddNonInheritableTag(kTracingUri, request.GetUrl());
  } catch (const std::exception& ex) {
    LOG_ERROR() << "can't finalize request processing: " << ex;
  }
}

TracingFactory::TracingFactory(const components::ComponentConfig& config,
                               const components::ComponentContext& context)
    : HttpMiddlewareFactoryBase{config, context},
      tracing_manager_{
          context.FindComponent<tracing::DefaultTracingManagerLocator>()
              .GetTracingManager()} {}

std::unique_ptr<HttpMiddlewareBase> TracingFactory::Create(
    const handlers::HttpHandlerBase& handler, yaml_config::YamlConfig) const {
  return std::make_unique<Tracing>(tracing_manager_, handler);
}

}  // namespace server::middlewares

USERVER_NAMESPACE_END
