#include <userver/server/handlers/http_handler_base.hpp>

#include <fmt/format.h>
#include <boost/algorithm/string/split.hpp>

#include <compression/gzip.hpp>
#include <server/handlers/http_handler_base_statistics.hpp>
#include <server/handlers/http_server_settings.hpp>
#include <server/http/http_request_impl.hpp>
#include <userver/components/component.hpp>
#include <userver/components/statistics_storage.hpp>
#include <userver/dynamic_config/storage/component.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/engine/task/inherited_variable.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/hostinfo/blocking/get_hostname.hpp>
#include <userver/http/common_headers.hpp>
#include <userver/logging/level_serialization.hpp>
#include <userver/logging/log.hpp>
#include <userver/server/component.hpp>
#include <userver/server/handlers/auth/auth_checker_settings_component.hpp>
#include <userver/server/http/http_error.hpp>
#include <userver/server/http/http_method.hpp>
#include <userver/server/http/http_response_body_stream.hpp>
#include <userver/server/request/task_inherited_data.hpp>
#include <userver/server/server_config.hpp>
#include <userver/tracing/set_throttle_reason.hpp>
#include <userver/tracing/span.hpp>
#include <userver/tracing/tags.hpp>
#include <userver/tracing/tracing.hpp>
#include <userver/utils/fast_scope_guard.hpp>
#include <userver/utils/from_string.hpp>
#include <userver/utils/graphite.hpp>
#include <userver/utils/log.hpp>
#include <userver/utils/overloaded.hpp>
#include <userver/utils/scope_guard.hpp>
#include <userver/utils/statistics/metadata.hpp>
#include <userver/utils/text.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

#include "auth/auth_checker.hpp"

USERVER_NAMESPACE_BEGIN

namespace server::handlers {
namespace {

const std::string kHostname = hostinfo::blocking::GetRealHostName();

// "request" is redundant: https://st.yandex-team.ru/TAXICOMMON-1793
// set to 1 if you need server metrics
constexpr bool kIncludeServerHttpMetrics = false;

template <typename HeadersHolder>
std::string GetHeadersLogString(const HeadersHolder& headers_holder) {
  formats::json::ValueBuilder json_headers(formats::json::Type::kObject);
  for (const auto& header_name : headers_holder.GetHeaderNames()) {
    json_headers[header_name] = headers_holder.GetHeader(header_name);
  }
  return formats::json::ToString(json_headers.ExtractValue());
}

std::vector<http::HttpMethod> InitAllowedMethods(const HandlerConfig& config) {
  std::vector<http::HttpMethod> allowed_methods;
  std::vector<std::string> methods;
  boost::split(methods, config.method, [](char c) { return c == ','; });
  for (const auto& method_str : methods) {
    auto method = http::HttpMethodFromString(method_str);
    if (!http::IsHandlerMethod(method)) {
      throw std::runtime_error(method_str + " is not supported in method list");
    }
    allowed_methods.push_back(method);
  }
  return allowed_methods;
}

void SetFormattedErrorResponse(http::HttpResponse& http_response,
                               FormattedErrorData&& formatted_error_data) {
  http_response.SetData(std::move(formatted_error_data.external_body));
  if (formatted_error_data.content_type) {
    http_response.SetContentType(*std::move(formatted_error_data.content_type));
  }
}

const std::string kTracingTypeResponse = "response";
const std::string kTracingTypeRequest = "request";
const std::string kTracingBody = "body";
const std::string kTracingUri = "uri";

const std::string kUserAgentTag = "useragent";
const std::string kAcceptLanguageTag = "acceptlang";

class RequestProcessor final {
 public:
  RequestProcessor(const HttpHandlerBase& handler,
                   const http::HttpRequestImpl& http_request_impl,
                   const http::HttpRequest& http_request,
                   request::RequestContext& context, bool log_request,
                   bool log_request_headers)
      : handler_(handler),
        http_request_impl_(http_request_impl),
        http_request_(http_request),
        context_(context),
        log_request_(log_request),
        log_request_headers_(log_request_headers) {}

  ~RequestProcessor() {
    try {
      auto& span = tracing::Span::CurrentSpan();
      auto& response = http_request_.GetHttpResponse();
      response.SetHeader(USERVER_NAMESPACE::http::headers::kXYaRequestId,
                         span.GetLink());

      const auto status_code = response.GetStatus();
      span.SetLogLevel(handler_.GetLogLevelForResponseStatus(status_code));
      if (!span.ShouldLogDefault()) {
        return;
      }

      int response_code = static_cast<int>(status_code);
      span.AddTag(tracing::kHttpStatusCode, response_code);
      if (response_code >= 500) span.AddTag(tracing::kErrorFlag, true);

      if (log_request_) {
        if (log_request_headers_) {
          span.AddNonInheritableTag("response_headers",
                                    GetHeadersLogString(response));
        }
        span.AddNonInheritableTag(
            kTracingBody, handler_.GetResponseDataForLoggingChecked(
                              http_request_, context_, response.GetData()));
      }
      span.AddNonInheritableTag(kTracingUri, http_request_.GetUrl());
    } catch (const std::exception& ex) {
      LOG_ERROR() << "can't finalize request processing: " << ex.what();
    }
  }

  template <typename Func>
  bool ProcessRequestStep(const std::string& step_name,
                          const Func& process_step_func) {
    if (process_finished_) return true;
    process_finished_ = DoProcessRequestStep(step_name, process_step_func);
    return process_finished_;
  }

 private:
  template <typename Func>
  bool DoProcessRequestStep(const std::string& step_name,
                            const Func& process_step_func) {
    const tracing::ScopeTime scope_time{"http_" + step_name};
    auto& response = http_request_.GetHttpResponse();

    try {
      process_step_func();
    } catch (const CustomHandlerException& ex) {
      auto http_status = http::GetHttpStatus(ex.GetCode());
      auto level = handler_.GetLogLevelForResponseStatus(http_status);
      LOG(level) << "custom handler exception in '" << handler_.HandlerName()
                 << "' handler in " + step_name + ": msg=" << ex;
      response.SetStatus(http_status);
      if (ex.IsExternalErrorBodyFormatted()) {
        response.SetData(ex.GetExternalErrorBody());
      } else {
        SetFormattedErrorResponse(response,
                                  handler_.GetFormattedExternalErrorBody(ex));
      }
      for (const auto& [name, value] : ex.GetExtraHeaders()) {
        response.SetHeader(name, value);
      }
      return true;
    } catch (const std::exception& ex) {
      if (engine::current_task::ShouldCancel()) {
        LOG_WARNING() << "request task cancelled, exception in '"
                      << handler_.HandlerName()
                      << "' handler in " + step_name + ": " << ex.what();
        response.SetStatus(http::HttpStatus::kClientClosedRequest);
      } else {
        LOG_ERROR() << "exception in '" << handler_.HandlerName()
                    << "' handler in " + step_name + ": " << ex;
        http_request_impl_.MarkAsInternalServerError();
        SetFormattedErrorResponse(response,
                                  handler_.GetFormattedExternalErrorBody({
                                      ExternalBody{response.GetData()},
                                      HandlerErrorCode::kServerSideError,
                                  }));
      }
      return true;
    } catch (...) {
      LOG_WARNING() << "unknown exception in '" << handler_.HandlerName()
                    << "' handler in " + step_name + " (task cancellation?)";
      response.SetStatus(http::HttpStatus::kClientClosedRequest);
      throw;
    }

    return false;
  }

  const HttpHandlerBase& handler_;
  const http::HttpRequestImpl& http_request_impl_;
  const http::HttpRequest& http_request_;
  bool process_finished_{false};

  request::RequestContext& context_;
  const bool log_request_;
  const bool log_request_headers_;
};

std::optional<std::chrono::milliseconds> ParseTimeout(
    const http::HttpRequest& request) {
  const auto& timeout_ms_str = request.GetHeader(
      USERVER_NAMESPACE::http::headers::kXYaTaxiClientTimeoutMs);
  if (timeout_ms_str.empty()) return std::nullopt;

  LOG_DEBUG() << "Got client timeout_ms=" << timeout_ms_str;
  std::chrono::milliseconds timeout;
  try {
    timeout = std::chrono::milliseconds{
        utils::FromString<std::uint64_t>(timeout_ms_str)};
  } catch (const std::exception& ex) {
    LOG_LIMITED_WARNING() << "Can't parse client timeout from '"
                          << timeout_ms_str << '\'';
    return std::nullopt;
  }

  // Very large timeouts may cause overflows.
  if (timeout >= std::chrono::hours{24 * 365 * 10}) {
    LOG_LIMITED_WARNING() << "Unreasonably large timeout: " << timeout;
    return std::nullopt;
  }

  return timeout;
}

void SetDeadlineInfoForRequest(const http::HttpRequest& request,
                               std::chrono::steady_clock::time_point start_time,
                               const HttpServerSettings& settings,
                               request::TaskInheritedData& info) {
  info.start_time = start_time;

  const auto timeout = ParseTimeout(request);
  if (!timeout) return;
  const auto deadline = engine::Deadline::FromTimePoint(start_time + *timeout);

  info.deadline = deadline;
  if (settings.need_cancel_handle_request_by_deadline) {
    engine::current_task::SetDeadline(deadline);
  }
}

void SetDeadlineTags(tracing::Span& span,
                     const request::TaskInheritedData& info) noexcept {
  if (!info.deadline.IsReachable()) return;

  const bool cancelled_by_deadline =
      engine::current_task::CancellationReason() ==
      engine::TaskCancellationReason::kDeadline;

  span.AddNonInheritableTag("deadline-received", info.deadline.IsReachable());
  span.AddNonInheritableTag("cancelled-by-deadline", cancelled_by_deadline);
}

std::string CutTrailingSlash(
    std::string meta_type,
    server::handlers::UrlTrailingSlashOption trailing_slash) {
  if (trailing_slash == UrlTrailingSlashOption::kBoth && meta_type.size() > 1 &&
      meta_type.back() == '/') {
    meta_type.pop_back();
  }

  return meta_type;
}

// Separate function to avoid heavy computations when the result is not going
// to be logged
logging::LogExtra LogRequestExtra(bool need_log_request_headers,
                                  const http::HttpRequest& http_request,
                                  const std::string& meta_type,
                                  const std::string& body_to_log,
                                  std::uint64_t body_length) {
  logging::LogExtra log_extra;

  if (need_log_request_headers) {
    log_extra.Extend("request_headers", GetHeadersLogString(http_request));
  }
  log_extra.Extend(tracing::kHttpMetaType, meta_type);
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

HttpHandlerBase::HttpHandlerBase(const components::ComponentConfig& config,
                                 const components::ComponentContext& context,
                                 bool is_monitor)
    : HandlerBase(config, context, is_monitor),
      config_source_(
          context.FindComponent<components::DynamicConfig>().GetSource()),
      allowed_methods_(InitAllowedMethods(GetConfig())),
      handler_name_(config.Name()),
      handler_statistics_(std::make_unique<HttpHandlerStatistics>()),
      request_statistics_(std::make_unique<HttpRequestStatistics>()),
      auth_checkers_(auth::CreateAuthCheckers(
          context, GetConfig(),
          context.FindComponent<components::AuthCheckerSettings>().Get())),
      log_level_(config["log-level"].As<std::optional<logging::Level>>()),
      rate_limit_(utils::TokenBucket::MakeUnbounded()),
      is_body_streamed_(config["response-body-stream"].As<bool>(false)) {
  if (allowed_methods_.empty()) {
    LOG_WARNING() << "empty allowed methods list in " << config.Name();
  }

  if (GetConfig().max_requests_per_second) {
    const auto max_rps = *GetConfig().max_requests_per_second;
    UASSERT_MSG(
        max_rps > 0,
        "max_requests_per_second option was not verified in config parsing");
    rate_limit_.SetMaxSize(max_rps);
    rate_limit_.SetRefillPolicy(
        {1, utils::TokenBucket::Duration{std::chrono::seconds(1)} / max_rps});
  }

  auto& server_component = context.FindComponent<components::Server>();

  engine::TaskProcessor& task_processor =
      context.GetTaskProcessor(GetConfig().task_processor);
  try {
    server_component.AddHandler(*this, task_processor);
  } catch (const std::exception& ex) {
    throw std::runtime_error(std::string("can't add handler to server: ") +
                             ex.what());
  }
  /// TODO: unable to add prefix metadata ATM

  const auto graphite_subpath = std::visit(
      utils::Overloaded{[](const std::string& path) {
                          return "by-path." + utils::graphite::EscapeName(path);
                        },
                        [](FallbackHandler fallback) {
                          return "by-fallback." + ToString(fallback);
                        }},
      GetConfig().path);
  const auto graphite_path =
      fmt::format("http.{}.by-handler.{}", graphite_subpath, config.Name());

  auto& statistics_storage =
      context.FindComponent<components::StatisticsStorage>().GetStorage();
  statistics_holder_ = statistics_storage.RegisterExtender(
      graphite_path,
      [this](const utils::statistics::StatisticsRequest& request) {
        return ExtendStatistics(request);
      });

  set_response_server_hostname_ =
      GetConfig().set_response_server_hostname.value_or(
          server_component.GetServer()
              .GetConfig()
              .set_response_server_hostname);
}

HttpHandlerBase::~HttpHandlerBase() { statistics_holder_.Unregister(); }

void HttpHandlerBase::HandleRequest(request::RequestBase& request,
                                    request::RequestContext& context) const {
  auto& http_request_impl = dynamic_cast<http::HttpRequestImpl&>(request);
  http::HttpRequest http_request(http_request_impl);
  auto& response = http_request.GetHttpResponse();

  try {
    HttpHandlerStatisticsScope stats_scope(*handler_statistics_,
                                           http_request.GetMethod(), response);

    const auto server_settings = config_source_.GetCopy(kHttpServerSettings);

    const auto& config = GetConfig();
    request::TaskInheritedData inherited_data{
        std::get_if<std::string>(&config.path), http_request.GetMethodStr(),
        /*start_time*/ {}, engine::Deadline{}};
    SetDeadlineInfoForRequest(http_request, request.StartTime(),
                              server_settings, inherited_data);
    request::kTaskInheritedData.Set(inherited_data);

    const auto& parent_link =
        http_request.GetHeader(USERVER_NAMESPACE::http::headers::kXYaRequestId);
    const auto& trace_id =
        http_request.GetHeader(USERVER_NAMESPACE::http::headers::kXYaTraceId);
    const auto& parent_span_id =
        http_request.GetHeader(USERVER_NAMESPACE::http::headers::kXYaSpanId);

    auto span = tracing::Span::MakeSpan(fmt::format("http/{}", HandlerName()),
                                        trace_id, parent_span_id);

    response.SetHeader(USERVER_NAMESPACE::http::headers::kXYaTraceId,
                       span.GetTraceId());
    response.SetHeader(USERVER_NAMESPACE::http::headers::kXYaSpanId,
                       span.GetSpanId());

    span.SetLocalLogLevel(log_level_);

    if (!parent_link.empty()) span.SetParentLink(parent_link);

    const auto meta_type = CutTrailingSlash(GetMetaType(http_request),
                                            GetConfig().url_trailing_slash);

    span.AddNonInheritableTag(tracing::kHttpMetaType, meta_type);
    span.AddNonInheritableTag(tracing::kType, kTracingTypeResponse);
    span.AddNonInheritableTag(tracing::kHttpMethod,
                              http_request.GetMethodStr());
    utils::FastScopeGuard deadline_tags_guard(
        [&]() noexcept { SetDeadlineTags(span, inherited_data); });

    static const std::string kParseRequestDataStep = "parse_request_data";
    static const std::string kCheckAuthStep = "check_auth";
    static const std::string kCheckRatelimitStep = "check_ratelimit";
    static const std::string kHandleRequestStep = "handle_request";
    static const std::string kDecompressRequestBody = "decompress_request_body";

    RequestProcessor request_processor(
        *this, http_request_impl, http_request, context,
        server_settings.need_log_request,
        server_settings.need_log_request_headers);

    request_processor.ProcessRequestStep(
        kCheckRatelimitStep,
        [this, &http_request] { return CheckRatelimit(http_request); });

    request_processor.ProcessRequestStep(
        kCheckAuthStep,
        [this, &http_request, &context] { CheckAuth(http_request, context); });

    if (GetConfig().decompress_request) {
      request_processor.ProcessRequestStep(
          kDecompressRequestBody,
          [this, &http_request] { DecompressRequestBody(http_request); });
    }

    request_processor.ProcessRequestStep(
        kParseRequestDataStep, [this, &http_request, &context] {
          ParseRequestData(http_request, context);
        });

    if (server_settings.need_log_request) {
      LOG_INFO() << "start handling"
                 << LogRequestExtra(
                        server_settings.need_log_request_headers, http_request,
                        meta_type,
                        GetRequestBodyForLoggingChecked(
                            http_request, context, http_request.RequestBody()),
                        http_request.RequestBody().length());
    }

    request_processor.ProcessRequestStep(
        kHandleRequestStep, [this, &response, &http_request, &context] {
          if (response.IsBodyStreamed()) {
            auto& response = http_request.GetHttpResponse();
            utils::ScopeGuard scope([&response] { response.SetHeadersEnd(); });

            HandleStreamRequest(
                http_request, context,
                http::ResponseBodyStream{response.GetBodyProducer(),
                                         http_request.GetHttpResponse()});
            // BodyProducer is dead
          } else {
            // !IsBodyStreamed()
            response.SetData(HandleRequestThrow(http_request, context));
          }
        });
  } catch (const std::exception& ex) {
    LOG_ERROR() << "unable to handle request: " << ex;
  }

  SetResponseAcceptEncoding(response);
  SetResponseServerHostname(response);
}

void HttpHandlerBase::ThrowUnsupportedHttpMethod(
    const http::HttpRequest& request) const {
  throw ClientError(
      HandlerErrorCode::kInvalidUsage,
      InternalMessage{fmt::format("method {} is not allowed in {}",
                                  request.GetMethodStr(), HandlerName())});
}

std::string HttpHandlerBase::HandleRequestThrow(
    const http::HttpRequest&, request::RequestContext&) const {
  throw std::runtime_error(
      "non-stream HandleRequestThrow() is executed, but the handler doesn't "
      "override HandleRequestThrow().");
}

void HttpHandlerBase::HandleStreamRequest(
    const server::http::HttpRequest&, server::request::RequestContext&,
    server::http::ResponseBodyStream&&) const {
  throw std::runtime_error(
      "stream HandleStreamRequest() is executed, but the handler doesn't "
      "override HandleStreamRequest().");
}

void HttpHandlerBase::ReportMalformedRequest(
    request::RequestBase& request) const {
  try {
    auto& http_request_impl = dynamic_cast<http::HttpRequestImpl&>(request);
    const http::HttpRequest http_request(http_request_impl);
    auto& response = http_request.GetHttpResponse();

    SetFormattedErrorResponse(
        response,
        GetFormattedExternalErrorBody({ExternalBody{response.GetData()},
                                       HandlerErrorCode::kRequestParseError}));
  } catch (const std::exception& ex) {
    LOG_ERROR() << "unable to handle ready request: " << ex;
  }
}

const std::string& HttpHandlerBase::HandlerName() const {
  return handler_name_;
}

const std::vector<http::HttpMethod>& HttpHandlerBase::GetAllowedMethods()
    const {
  return allowed_methods_;
}

HttpRequestStatistics& HttpHandlerBase::GetRequestStatistics() const {
  return *request_statistics_;
}

logging::Level HttpHandlerBase::GetLogLevelForResponseStatus(
    http::HttpStatus status) const {
  auto status_code = static_cast<int>(status);
  if (status_code >= 400 && status_code <= 499) return logging::Level::kWarning;
  if (status_code >= 500 && status_code <= 599) return logging::Level::kError;
  return logging::Level::kInfo;
}

FormattedErrorData HttpHandlerBase::GetFormattedExternalErrorBody(
    const CustomHandlerException& exc) const {
  return {exc.GetExternalErrorBody()};
}

void HttpHandlerBase::CheckAuth(const http::HttpRequest& http_request,
                                request::RequestContext& context) const {
  if (!config_source_.GetCopy(kHttpServerSettings)
           .need_check_auth_in_handlers) {
    LOG_DEBUG() << "auth checks are disabled for current service";
    return;
  }

  if (!NeedCheckAuth()) {
    LOG_DEBUG() << "auth checks are disabled for current handler";
    return;
  }

  auth::CheckAuth(auth_checkers_, http_request, context);
}

void HttpHandlerBase::CheckRatelimit(
    const http::HttpRequest& http_request) const {
  auto& statistics = handler_statistics_->GetByMethod(http_request.GetMethod());
  auto& total_statistics = handler_statistics_->GetTotal();

  const bool success = rate_limit_.Obtain();
  if (!success) {
    UASSERT(GetConfig().max_requests_per_second);

    auto& http_response = http_request.GetHttpResponse();
    auto log_reason =
        fmt::format("reached max_requests_per_second={}",
                    GetConfig().max_requests_per_second.value_or(0));
    SetThrottleReason(
        http_response, std::move(log_reason),
        USERVER_NAMESPACE::http::headers::ratelimit_reason::kGlobal);
    statistics.IncrementRateLimitReached();
    total_statistics.IncrementRateLimitReached();

    throw ExceptionWithCode<HandlerErrorCode::kTooManyRequests>();
  }

  auto max_requests_in_flight = GetConfig().max_requests_in_flight;
  auto requests_in_flight = statistics.GetInFlight();
  if (max_requests_in_flight &&
      (requests_in_flight > *max_requests_in_flight)) {
    auto& http_response = http_request.GetHttpResponse();
    auto log_reason = fmt::format("reached max_requests_in_flight={}",
                                  *max_requests_in_flight);
    SetThrottleReason(
        http_response, std::move(log_reason),
        USERVER_NAMESPACE::http::headers::ratelimit_reason::kInFlight);

    statistics.IncrementTooManyRequestsInFlight();
    total_statistics.IncrementTooManyRequestsInFlight();

    throw ExceptionWithCode<HandlerErrorCode::kTooManyRequests>();
  }
}

void HttpHandlerBase::DecompressRequestBody(
    http::HttpRequest& http_request) const {
  if (!http_request.IsBodyCompressed()) return;

  const auto& content_encoding = http_request.GetHeader("Content-Encoding");

  try {
    if (content_encoding == "gzip") {
      auto body = compression::gzip::Decompress(http_request.RequestBody(),
                                                GetConfig().max_request_size);
      http_request.SetRequestBody(std::move(body));
      if (GetConfig().parse_args_from_body.value_or(false)) {
        http_request.ParseArgsFromBody();
      }
      return;
    }
  } catch (const compression::TooBigError& e) {
    throw ClientError(HandlerErrorCode::kPayloadTooLarge);
  } catch (const std::exception& e) {
    throw RequestParseError(InternalMessage{
        std::string{"Failed to decompress request body: "} + e.what()});
  }

  auto& response = http_request.GetHttpResponse();
  SetResponseAcceptEncoding(response);
  throw ClientError(HandlerErrorCode::kUnsupportedMediaType);
}

std::string HttpHandlerBase::GetRequestBodyForLogging(
    const http::HttpRequest&, request::RequestContext&,
    const std::string& request_body) const {
  size_t limit = GetConfig().request_body_size_log_limit;
  return utils::log::ToLimitedUtf8(request_body, limit);
}

std::string HttpHandlerBase::GetResponseDataForLogging(
    const http::HttpRequest&, request::RequestContext&,
    const std::string& response_data) const {
  size_t limit = GetConfig().response_data_size_log_limit;
  return utils::log::ToLimitedUtf8(response_data, limit);
}

std::string HttpHandlerBase::GetMetaType(
    const http::HttpRequest& request) const {
  return request.GetRequestPath();
}

std::string HttpHandlerBase::GetRequestBodyForLoggingChecked(
    const http::HttpRequest& request, request::RequestContext& context,
    const std::string& request_body) const {
  try {
    return GetRequestBodyForLogging(request, context, request_body);
  } catch (const std::exception& ex) {
    LOG_LIMITED_ERROR() << "failed to get request body for logging: " << ex;
    return "<error in GetRequestBodyForLogging>";
  }
}

std::string HttpHandlerBase::GetResponseDataForLoggingChecked(
    const http::HttpRequest& request, request::RequestContext& context,
    const std::string& response_data) const {
  try {
    return GetResponseDataForLogging(request, context, response_data);
  } catch (const std::exception& ex) {
    LOG_LIMITED_ERROR() << "failed to get response data for logging: " << ex;
    return "<error in GetResponseDataForLogging>";
  }
}

template <typename HttpStatistics>
formats::json::ValueBuilder HttpHandlerBase::FormatStatistics(
    const HttpStatistics& stats) {
  formats::json::ValueBuilder result;
  result["all-methods"] = stats.GetTotal();
  utils::statistics::SolomonSkip(result["all-methods"]);

  if (IsMethodStatisticIncluded()) {
    formats::json::ValueBuilder by_method;
    for (auto method : GetAllowedMethods()) {
      by_method[ToString(method)] = stats.GetByMethod(method);
    }
    utils::statistics::SolomonChildrenAreLabelValues(by_method, "http_method");
    utils::statistics::SolomonSkip(by_method);
    result["by-method"] = std::move(by_method);
  }
  return result;
}

formats::json::ValueBuilder HttpHandlerBase::ExtendStatistics(
    const utils::statistics::StatisticsRequest& /*request*/) {
  formats::json::ValueBuilder result;
  result["handler"] = FormatStatistics(*handler_statistics_);

  if constexpr (kIncludeServerHttpMetrics) {
    result["request"] = FormatStatistics(*request_statistics_);
  }

  return result;
}

void HttpHandlerBase::SetResponseAcceptEncoding(
    http::HttpResponse& response) const {
  if (!GetConfig().decompress_request) return;

  // RFC7694, 3.
  // This specification expands that definition to allow "Accept-Encoding"
  // as a response header field as well.  When present in a response, it
  // indicates what content codings the resource was willing to accept in
  // the associated request.

  if (response.GetStatus() == http::HttpStatus::kUnsupportedMediaType) {
    // User has already set the Accept-Encoding
    return;
  }

  if (!response.HasHeader(USERVER_NAMESPACE::http::headers::kAcceptEncoding)) {
    response.SetHeader(USERVER_NAMESPACE::http::headers::kAcceptEncoding,
                       "gzip, identity");
  }
}

void HttpHandlerBase::SetResponseServerHostname(
    http::HttpResponse& response) const {
  if (set_response_server_hostname_) {
    response.SetHeader(USERVER_NAMESPACE::http::headers::kXYaTaxiServerHostname,
                       kHostname);
  }
}

yaml_config::Schema HttpHandlerBase::GetStaticConfigSchema() {
  return yaml_config::MergeSchemas<HandlerBase>(R"(
type: object
description: Base class for all the userver HTTP handlers
additionalProperties: false
properties:
    log-level:
        type: string
        description: overrides log level for this handle
        defaultDescription: <no override>
)");
}

}  // namespace server::handlers

USERVER_NAMESPACE_END
