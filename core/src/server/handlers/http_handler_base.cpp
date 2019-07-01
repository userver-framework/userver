#include <server/handlers/http_handler_base.hpp>

#include <boost/algorithm/string/split.hpp>

#include <components/statistics_storage.hpp>
#include <formats/json/serialize.hpp>
#include <formats/json/value_builder.hpp>
#include <http/common_headers.hpp>
#include <logging/log.hpp>
#include <server/component.hpp>
#include <server/handlers/http_handler_base_statistics.hpp>
#include <server/http/http_error.hpp>
#include <server/http/http_method.hpp>
#include <server/http/http_request_impl.hpp>
#include <tracing/span.hpp>
#include <tracing/tags.hpp>
#include <tracing/tracing.hpp>
#include <utils/graphite.hpp>
#include <utils/statistics/percentile_format_json.hpp>
#include <utils/text.hpp>

#include "auth/auth_checker.hpp"

namespace server {
namespace handlers {
namespace {

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
  auto& method_list = config.method;

  if (method_list) {
    std::vector<std::string> methods;
    boost::split(methods, *method_list, [](char c) { return c == ','; });
    for (const auto& method_str : methods) {
      auto method = http::HttpMethodFromString(method_str);
      if (!http::IsHandlerMethod(method)) {
        throw std::runtime_error(method_str +
                                 " is not supported in method list");
      }
      allowed_methods.push_back(method);
    }
  } else {
    for (auto method : http::kHandlerMethods) {
      allowed_methods.push_back(method);
    }
  }
  return allowed_methods;
}

class RequestProcessor final {
 public:
  RequestProcessor(const HttpHandlerBase& handler,
                   const http::HttpRequestImpl& http_request_impl,
                   const http::HttpRequest& http_request)
      : handler_(handler),
        http_request_impl_(http_request_impl),
        http_request_(http_request),
        process_finished_(false) {}

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
    auto& response = http_request_.GetHttpResponse();

    try {
      process_step_func();
    } catch (const http::HttpException& ex) {
      // TODO Remove this catch branch
      auto level = handler_.GetLogLevelForResponseStatus(ex.GetStatus());
      LOG(level) << "http exception in '" << handler_.HandlerName()
                 << "' handler in " + step_name + ": code="
                 << HttpStatusString(ex.GetStatus()) << ", msg=" << ex
                 << ", body=" << ex.GetExternalErrorBody();
      response.SetStatus(ex.GetStatus());
      response.SetData(ex.GetExternalErrorBody());
      return true;
    } catch (const CustomHandlerException& ex) {
      auto http_status = http::GetHttpStatus(ex.GetCode());
      auto level = handler_.GetLogLevelForResponseStatus(http_status);
      LOG(level) << "custom handler exception in '" << handler_.HandlerName()
                 << "' handler in " + step_name + ": msg=" << ex
                 << ", body=" << ex.GetExternalErrorBody();
      response.SetStatus(http_status);
      response.SetData(handler_.GetFormattedExternalErrorBody(
          response.GetStatus(), ex.GetExternalErrorBody()));
      return true;
    } catch (const std::exception& ex) {
      LOG_ERROR() << "exception in '" << handler_.HandlerName()
                  << "' handler in " + step_name + ": " << ex;
      http_request_impl_.MarkAsInternalServerError();
      response.SetData(handler_.GetFormattedExternalErrorBody(
          response.GetStatus(), response.GetData()));
      return true;
    }

    return false;
  }

  const HttpHandlerBase& handler_;
  const http::HttpRequestImpl& http_request_impl_;
  const http::HttpRequest& http_request_;
  bool process_finished_;
};

const std::string kTracingTypeResponse = "response";
const std::string kTracingTypeRequest = "request";
const std::string kTracingBody = "body";
const std::string kTracingUri = "uri";

}  // namespace

formats::json::ValueBuilder HttpHandlerBase::StatisticsToJson(
    const HttpHandlerMethodStatistics& stats) {
  formats::json::ValueBuilder result;
  formats::json::ValueBuilder total;

  total["reply-codes"] = stats.FormatReplyCodes();
  total["in-flight"] = stats.GetInFlight();

  total["timings"]["1min"] =
      utils::statistics::PercentileToJson(stats.GetTimings());

  result["total"] = std::move(total);
  return result;
}

HttpHandlerBase::HttpHandlerBase(
    const components::ComponentConfig& config,
    const components::ComponentContext& component_context, bool is_monitor)
    : HandlerBase(config, component_context, is_monitor),
      http_server_settings_(
          component_context
              .FindComponent<components::HttpServerSettingsBase>()),
      allowed_methods_(InitAllowedMethods(GetConfig())),
      statistics_storage_(
          component_context.FindComponent<components::StatisticsStorage>()),
      handler_statistics_(std::make_unique<HttpHandlerStatistics>()),
      request_statistics_(std::make_unique<HttpHandlerStatistics>()),
      auth_checkers_(auth::CreateAuthCheckers(
          component_context, GetConfig(),
          http_server_settings_.GetAuthCheckerSettings())),
      log_level_(logging::OptionalLevelFromString(
          config.ParseOptionalString("log-level"))) {
  if (allowed_methods_.empty()) {
    LOG_WARNING() << "empty allowed methods list in " << config.Name();
  }

  if (IsEnabled()) {
    auto& server_component =
        component_context.FindComponent<components::Server>();

    engine::TaskProcessor& task_processor =
        component_context.GetTaskProcessor(GetConfig().task_processor);
    try {
      server_component.AddHandler(*this, task_processor);
    } catch (const std::exception& ex) {
      throw std::runtime_error(std::string("can't add handler to server: ") +
                               ex.what());
    }
    const auto graphite_path = "http.by-path." +
                               utils::graphite::EscapeName(GetConfig().path) +
                               ".by-handler." + config.Name();
    statistics_holder_ = statistics_storage_.GetStorage().RegisterExtender(
        graphite_path, std::bind(&HttpHandlerBase::ExtendStatistics, this,
                                 std::placeholders::_1));
  }
}

HttpHandlerBase::~HttpHandlerBase() { statistics_holder_.Unregister(); }

void HttpHandlerBase::HandleRequest(const request::RequestBase& request,
                                    request::RequestContext& context) const {
  try {
    const auto& http_request_impl =
        dynamic_cast<const http::HttpRequestImpl&>(request);
    const http::HttpRequest http_request(http_request_impl);
    auto& response = http_request.GetHttpResponse();
    const auto start_time = std::chrono::system_clock::now();

    bool log_request = http_server_settings_.NeedLogRequest();
    bool log_request_headers = http_server_settings_.NeedLogRequestHeaders();

    const auto& parent_link =
        http_request.GetHeader(::http::headers::kXYaRequestId);
    const auto& trace_id = http_request.GetHeader(::http::headers::kXYaTraceId);
    const auto& parent_span_id =
        http_request.GetHeader(::http::headers::kXYaSpanId);

    auto span = tracing::Span::MakeSpan("http/" + HandlerName(), trace_id,
                                        parent_span_id);

    span.SetLocalLogLevel(log_level_);

    if (!parent_link.empty()) span.AddTag("parent_link", parent_link);
    span.AddNonInheritableTag(tracing::kHttpMetaType,
                              http_request.GetRequestPath());
    span.AddNonInheritableTag(tracing::kType, kTracingTypeResponse);
    span.AddNonInheritableTag(tracing::kHttpMethod,
                              http_request.GetMethodStr());

    static const std::string kParseRequestDataStep = "parse_request_data";
    static const std::string kCheckAuthStep = "check_auth";
    static const std::string kHandleRequestStep = "handle_request";

    RequestProcessor request_processor(*this, http_request_impl, http_request);

    request_processor.ProcessRequestStep(
        kCheckAuthStep,
        [this, &http_request, &context] { CheckAuth(http_request, context); });

    request_processor.ProcessRequestStep(
        kParseRequestDataStep, [this, &http_request, &context] {
          ParseRequestData(http_request, context);
        });

    if (log_request) {
      logging::LogExtra log_extra;

      if (log_request_headers) {
        log_extra.Extend("request_headers", GetHeadersLogString(http_request));
      }
      log_extra.Extend(tracing::kType, kTracingTypeRequest);
      const auto& body = http_request.RequestBody();
      uint64_t body_length = body.length();
      log_extra.Extend("request_body_length", body_length);
      log_extra.Extend(kTracingBody, GetRequestBodyForLoggingChecked(
                                         http_request, context, body));
      log_extra.Extend(kTracingUri, http_request.GetUrl());
      LOG_INFO()
          << "start handling"
          // NOLINTNEXTLINE(bugprone-use-after-move,hicpp-invalid-access-moved)
          << std::move(log_extra);
    }

    HttpHandlerStatisticsScope stats_scope(*handler_statistics_,
                                           http_request.GetMethod());

    request_processor.ProcessRequestStep(
        kHandleRequestStep, [this, &response, &http_request, &context] {
          response.SetData(HandleRequestThrow(http_request, context));
        });

    response.SetHeader(::http::headers::kXYaRequestId, span.GetLink());
    int response_code = static_cast<int>(response.GetStatus());
    span.AddTag(tracing::kHttpStatusCode, response_code);
    if (response_code >= 500) span.AddTag(tracing::kErrorFlag, true);
    span.SetLogLevel(GetLogLevelForResponseStatus(response.GetStatus()));

    if (log_request) {
      if (log_request_headers) {
        span.AddNonInheritableTag("response_headers",
                                  GetHeadersLogString(response));
      }
      span.AddNonInheritableTag(
          kTracingBody, GetResponseDataForLoggingChecked(http_request, context,
                                                         response.GetData()));
    }
    span.AddNonInheritableTag(kTracingUri, http_request.GetUrl());

    const auto finish_time = std::chrono::system_clock::now();
    const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        finish_time - start_time);
    stats_scope.Account(static_cast<int>(response.GetStatus()), ms);
  } catch (const std::exception& ex) {
    LOG_ERROR() << "unable to handle request: " << ex;
  }
}

void HttpHandlerBase::ThrowUnsupportedHttpMethod(
    const http::HttpRequest& request) const {
  throw ClientError(HandlerErrorCode::kInvalidUsage,
                    InternalMessage{"method " + request.GetMethodStr() +
                                    " is not allowed in " + HandlerName()});
}

void HttpHandlerBase::OnRequestComplete(
    const request::RequestBase& request,
    request::RequestContext& context) const {
  try {
    const http::HttpRequest http_request(
        dynamic_cast<const http::HttpRequestImpl&>(request));
    try {
      OnRequestCompleteThrow(http_request, context);
    } catch (const std::exception& ex) {
      LOG_ERROR() << "exception in '" << HandlerName()
                  << "' hander in on_request_complete: " << ex;
    }
  } catch (const std::exception& ex) {
    LOG_ERROR() << "unable to complete request: " << ex;
  }
}

void HttpHandlerBase::HandleReadyRequest(
    const request::RequestBase& request) const {
  try {
    const auto& http_request_impl =
        dynamic_cast<const http::HttpRequestImpl&>(request);
    const http::HttpRequest http_request(http_request_impl);
    auto& response = http_request.GetHttpResponse();

    response.SetData(GetFormattedExternalErrorBody(response.GetStatus(),
                                                   response.GetData()));
  } catch (const std::exception& ex) {
    LOG_ERROR() << "unable to handle ready request: " << ex;
  }
}

const std::vector<http::HttpMethod>& HttpHandlerBase::GetAllowedMethods()
    const {
  return allowed_methods_;
}

HttpHandlerStatistics& HttpHandlerBase::GetRequestStatistics() const {
  return *request_statistics_;
}

logging::Level HttpHandlerBase::GetLogLevelForResponseStatus(
    http::HttpStatus status) const {
  auto status_code = static_cast<int>(status);
  if (status_code >= 400 && status_code <= 499) return logging::Level::kWarning;
  if (status_code >= 500 && status_code <= 599) return logging::Level::kError;
  return logging::Level::kInfo;
}

std::string HttpHandlerBase::GetFormattedExternalErrorBody(
    http::HttpStatus, std::string external_error_body) const {
  return external_error_body;
}

void HttpHandlerBase::CheckAuth(const http::HttpRequest& http_request,
                                request::RequestContext& context) const {
  if (!http_server_settings_.NeedCheckAuthInHandlers()) {
    LOG_DEBUG() << "auth checks are disabled for current service";
    return;
  }

  if (!NeedCheckAuth()) {
    LOG_DEBUG() << "auth checks are disabled for current handler";
    return;
  }

  auth::CheckAuth(auth_checkers_, http_request, context);
}

std::string HttpHandlerBase::GetRequestBodyForLogging(
    const http::HttpRequest&, request::RequestContext&,
    const std::string& request_body) const {
  static const std::string kTruncated = "...(truncated)";
  size_t limit = GetConfig().request_body_size_log_limit;
  if (request_body.size() <= limit) return request_body;
  std::string result = request_body.substr(0, limit);
  utils::text::utf8::TrimTruncatedEnding(result);
  result += kTruncated;
  return result;
}

std::string HttpHandlerBase::GetResponseDataForLogging(
    const http::HttpRequest&, request::RequestContext&,
    const std::string& response_data) const {
  size_t limit = GetConfig().response_data_size_log_limit;
  if (response_data.size() <= limit) return response_data;
  return response_data.substr(0, limit) + "...(truncated, total " +
         std::to_string(response_data.size()) + " bytes)";
}

std::string HttpHandlerBase::GetRequestBodyForLoggingChecked(
    const http::HttpRequest& request, request::RequestContext& context,
    const std::string& request_body) const {
  try {
    return GetRequestBodyForLogging(request, context, request_body);
  } catch (const std::exception& ex) {
    LOG_ERROR() << "failed to get request body for logging: " << ex;
    return "<error in GetRequestBodyForLogging>";
  }
}

std::string HttpHandlerBase::GetResponseDataForLoggingChecked(
    const http::HttpRequest& request, request::RequestContext& context,
    const std::string& response_data) const {
  try {
    return GetResponseDataForLogging(request, context, response_data);
  } catch (const std::exception& ex) {
    LOG_ERROR() << "failed to get response data for logging: " << ex;
    return "<error in GetResponseDataForLogging>";
  }
}

formats::json::ValueBuilder HttpHandlerBase::ExtendStatistics(
    const utils::statistics::StatisticsRequest& /*request*/) {
  formats::json::ValueBuilder result;
  result["handler"] = FormatStatistics(*handler_statistics_);
  result["request"] = FormatStatistics(*request_statistics_);
  return result;
}

formats::json::ValueBuilder HttpHandlerBase::FormatStatistics(
    const HttpHandlerStatistics& stats) {
  formats::json::ValueBuilder result;
  result["all-methods"] = StatisticsToJson(stats.GetTotalStatistics());

  if (IsMethodStatisticIncluded()) {
    formats::json::ValueBuilder by_method;
    for (auto method : GetAllowedMethods()) {
      by_method[ToString(method)] =
          StatisticsToJson(stats.GetStatisticByMethod(method));
    }
    result["by-method"] = std::move(by_method);
  }
  return result;
}

}  // namespace handlers
}  // namespace server
