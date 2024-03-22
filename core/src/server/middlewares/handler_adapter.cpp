#include <server/middlewares/handler_adapter.hpp>

#include <server/handlers/http_server_settings.hpp>
#include <server/middlewares/misc.hpp>
#include <server/request/internal_request_context.hpp>

#include <userver/formats/json/string_builder.hpp>
#include <userver/formats/serialize/write_to_stream.hpp>
#include <userver/http/common_headers.hpp>
#include <userver/logging/log.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/server/http/http_request.hpp>
#include <userver/server/request/request_context.hpp>
#include <userver/tracing/tags.hpp>
#include <userver/utils/scope_guard.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::middlewares {

namespace {

const std::string kTracingTypeRequest = "request";
const std::string kTracingBody = "body";
const std::string kTracingUri = "uri";

const std::string kUserAgentTag = "useragent";
const std::string kAcceptLanguageTag = "acceptlang";

std::string GetHeadersLogString(const http::HttpRequest& request) {
  formats::json::StringBuilder sb{};
  WriteToStream(request.GetHeaders(), sb);
  return sb.GetString();
}

// Separate function to avoid heavy computations when the result is not going
// to be logged
logging::LogExtra LogRequestExtra(bool need_log_request_headers,
                                  const http::HttpRequest& http_request,
                                  std::string_view meta_type,
                                  const std::string& body_to_log,
                                  std::uint64_t body_length) {
  logging::LogExtra log_extra;

  if (need_log_request_headers) {
    log_extra.Extend("request_headers", GetHeadersLogString(http_request));
  }
  log_extra.Extend(tracing::kHttpMetaType, std::string{meta_type});
  log_extra.Extend(tracing::kType, kTracingTypeRequest);
  log_extra.Extend("request_body_length", body_length);
  log_extra.Extend(kTracingBody, body_to_log);
  log_extra.Extend(kTracingUri, http_request.GetUrl());
  log_extra.Extend(tracing::kHttpMethod, http_request.GetMethodStr());

  const auto& request_application = http_request.GetHeader(
      USERVER_NAMESPACE::http::headers::kXRequestApplication);
  if (!request_application.empty()) {
    log_extra.Extend("request_application", request_application);
  }

  const auto& user_agent =
      http_request.GetHeader(USERVER_NAMESPACE::http::headers::kUserAgent);
  if (!user_agent.empty()) {
    log_extra.Extend(kUserAgentTag, user_agent);
  }
  const auto& accept_language =
      http_request.GetHeader(USERVER_NAMESPACE::http::headers::kAcceptLanguage);
  if (!accept_language.empty()) {
    log_extra.Extend(kAcceptLanguageTag, accept_language);
  }

  return log_extra;
}

}  // namespace

HandlerAdapter::HandlerAdapter(const handlers::HttpHandlerBase& handler)
    : handler_{handler} {}

void HandlerAdapter::HandleRequest(http::HttpRequest& request,
                                   request::RequestContext& context) const {
  {
    // Logically speaking, the handler should be responsible for parsing request
    // data and not us, but we have to log request data AFTER it has been
    // parsed. Request parsing isn't optional, so we can't move it into its own
    // optional middleware, and this is the only place to hook the pipeline
    // reliably.
    const utils::ScopeGuard log_request_scope{
        [this, &request, &context] { LogRequest(request, context); }};

    ParseRequestData(request, context);
  }

  handler_.HandleHttpRequest(request, context);
}

void HandlerAdapter::ParseRequestData(const http::HttpRequest& request,
                                      request::RequestContext& context) const {
  const auto scope_time =
      tracing::ScopeTime::CreateOptionalScopeTime("http_parse_request_data");
  handler_.ParseRequestData(request, context);
}

void HandlerAdapter::LogRequest(const http::HttpRequest& request,
                                request::RequestContext& context) const {
  const auto& config_snapshot =
      context.GetInternalContext().GetConfigSnapshot();

  if (config_snapshot[handlers::kLogRequest]) {
    const bool need_log_request_headers =
        config_snapshot[handlers::kLogRequestHeaders];
    LOG_INFO() << "start handling"
               << LogRequestExtra(need_log_request_headers, request,
                                  misc::CutTrailingSlash(
                                      request.GetRequestPath(),
                                      handler_.GetConfig().url_trailing_slash),
                                  handler_.GetRequestBodyForLoggingChecked(
                                      request, context, request.RequestBody()),
                                  request.RequestBody().length());
  }
}

}  // namespace server::middlewares

USERVER_NAMESPACE_END
