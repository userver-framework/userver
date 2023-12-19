#include <userver/server/handlers/http_handler_base.hpp>

#include <fmt/core.h>
#include <boost/algorithm/string/split.hpp>

#include <compression/gzip.hpp>
#include <server/handlers/http_handler_base_statistics.hpp>
#include <server/handlers/http_server_settings.hpp>
#include <server/http/http_request_impl.hpp>
#include <server/server_config.hpp>
#include <userver/baggage/baggage.hpp>
#include <userver/baggage/baggage_settings.hpp>
#include <userver/components/component.hpp>
#include <userver/components/statistics_storage.hpp>
#include <userver/dynamic_config/storage/component.hpp>
#include <userver/engine/deadline.hpp>
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
#include <userver/server/handlers/impl/deadline_propagation_config.hpp>
#include <userver/server/http/http_error.hpp>
#include <userver/server/http/http_method.hpp>
#include <userver/server/http/http_response_body_stream.hpp>
#include <userver/server/http/http_status.hpp>
#include <userver/server/request/task_inherited_data.hpp>
#include <userver/tracing/manager_component.hpp>
#include <userver/tracing/set_throttle_reason.hpp>
#include <userver/tracing/span_builder.hpp>
#include <userver/tracing/tags.hpp>
#include <userver/tracing/tracing.hpp>
#include <userver/utils/algo.hpp>
#include <userver/utils/encoding/hex.hpp>
#include <userver/utils/fast_scope_guard.hpp>
#include <userver/utils/from_string.hpp>
#include <userver/utils/graphite.hpp>
#include <userver/utils/log.hpp>
#include <userver/utils/overloaded.hpp>
#include <userver/utils/scope_guard.hpp>
#include <userver/utils/text_light.hpp>
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

void SetFormattedErrorResponse(
    server::http::ResponseBodyStream& response_body_stream,
    FormattedErrorData&& formatted_error, engine::Deadline deadline) {
  if (formatted_error.content_type) {
    response_body_stream.SetHeader(
        USERVER_NAMESPACE::http::headers::kContentType,
        std::move(formatted_error.content_type->ToString()));
  }
  response_body_stream.SetEndOfHeaders();
  response_body_stream.PushBodyChunk(std::move(formatted_error.external_body),
                                     deadline);
}

void SetUpBaggage(const http::HttpRequest& http_request,
                  const dynamic_config::Snapshot& config_snapshot) {
  if (config_snapshot[baggage::kBaggageEnabled]) {
    auto baggage_header =
        http_request.GetHeader(USERVER_NAMESPACE::http::headers::kXBaggage);
    if (!baggage_header.empty()) {
      LOG_DEBUG() << "Got baggage header: " << baggage_header;
      const auto& baggage_settings = config_snapshot[baggage::kBaggageSettings];
      auto baggage = baggage::TryMakeBaggage(std::move(baggage_header),
                                             baggage_settings.allowed_keys);
      if (baggage) {
        baggage::kInheritedBaggage.Set(std::move(*baggage));
      }
    }
  }
}

void LogYandexHeaders(const http::HttpRequest& http_request) {
  const auto& yandex_request_id =
      http_request.GetHeader(USERVER_NAMESPACE::http::headers::kXRequestId);
  const auto& yandex_backend_server =
      http_request.GetHeader(USERVER_NAMESPACE::http::headers::kXBackendServer);
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
                   request::RequestContext& context,
                   dynamic_config::Snapshot&& snapshot)
      : handler_(handler),
        http_request_impl_(http_request_impl),
        http_request_(http_request),
        context_(context),
        config_snapshot_(std::move(snapshot)),
        need_log_response_(config_snapshot_[kLogRequest]),
        need_log_response_headers_(config_snapshot_[kLogRequestHeaders]) {}

  ~RequestProcessor() {
    try {
      auto& span = tracing::Span::CurrentSpan();
      auto& response = http_request_.GetHttpResponse();

      const auto status_code = response.GetStatus();
      span.SetLogLevel(
          forced_log_level_
              ? *forced_log_level_
              : handler_.GetLogLevelForResponseStatus(status_code));
      if (!span.ShouldLogDefault()) {
        return;
      }

      int response_code = static_cast<int>(status_code);
      span.AddTag(tracing::kHttpStatusCode, response_code);
      if (response_code >= 500) span.AddTag(tracing::kErrorFlag, true);

      if (need_log_response_) {
        if (need_log_response_headers_) {
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
  void ProcessRequestStep(std::string_view step_name,
                          const Func& process_step_func) {
    if (process_finished_) return;
    const tracing::ScopeTime scope_time{std::string{step_name}};
    DoProcessRequestStep(step_name, process_step_func);
  }

  template <typename Func>
  void ProcessRequestStepNoScopeTime(std::string_view step_name,
                                     const Func& process_step_func) {
    if (process_finished_) return;
    DoProcessRequestStep(step_name, process_step_func);
  }

  void HandleCustomException(std::string_view step_name,
                             const CustomHandlerException& ex) {
    process_finished_ = true;
    auto& response = http_request_.GetHttpResponse();
    auto http_status = http::GetHttpStatus(ex);
    auto level = handler_.GetLogLevelForResponseStatus(http_status);
    LOG(level) << "custom handler exception in '" << handler_.HandlerName()
               << "' handler in " << step_name << ": msg=" << ex;

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
  }

  void FinishProcessing() noexcept { process_finished_ = true; }

  const http::HttpRequest& GetRequest() { return http_request_; }

  const HttpHandlerBase& GetHandler() const { return handler_; }

  request::RequestContext& GetContext() { return context_; }

  const dynamic_config::Snapshot& GetInitialDynamicConfig() const {
    return config_snapshot_;
  }

  void ResetDynamicConfig() {
    [[maybe_unused]] const auto for_destruction = std::move(config_snapshot_);
  }

  void SetForcedLogLevel(logging::Level level) { forced_log_level_ = level; }

 private:
  template <typename Func>
  void DoProcessRequestStep(std::string_view step_name,
                            const Func& process_step_func) {
    try {
      process_step_func();
    } catch (const CustomHandlerException& ex) {
      HandleCustomException(step_name, ex);
    } catch (const std::exception& ex) {
      process_finished_ = true;
      auto& response = http_request_.GetHttpResponse();
      if (engine::current_task::ShouldCancel()) {
        LOG_WARNING() << "request task cancelled, exception in '"
                      << handler_.HandlerName() << "' handler in " << step_name
                      << ": " << ex.what();
        response.SetStatus(http::HttpStatus::kClientClosedRequest);
      } else {
        LOG_ERROR() << "exception in '" << handler_.HandlerName()
                    << "' handler in " << step_name << ": " << ex;
        http_request_impl_.MarkAsInternalServerError();
        SetFormattedErrorResponse(response,
                                  handler_.GetFormattedExternalErrorBody({
                                      HandlerErrorCode::kServerSideError,
                                      ExternalBody{response.GetData()},
                                  }));
      }
    } catch (...) {
      process_finished_ = true;
      auto& response = http_request_.GetHttpResponse();
      LOG_WARNING() << "unknown exception in '" << handler_.HandlerName()
                    << "' handler in " << step_name << " (task cancellation?)";
      response.SetStatus(http::HttpStatus::kClientClosedRequest);
      throw;
    }
  }

  const HttpHandlerBase& handler_;
  const http::HttpRequestImpl& http_request_impl_;
  const http::HttpRequest& http_request_;

  bool process_finished_{false};
  std::optional<logging::Level> forced_log_level_{};

  request::RequestContext& context_;
  dynamic_config::Snapshot config_snapshot_;
  const bool need_log_response_;
  const bool need_log_response_headers_;
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

struct DeadlinePropagationContext final {
  bool need_log_response{false};
  bool is_cancelled_by_deadline{false};
};

void HandleDeadlineExpired(RequestProcessor& processor,
                           DeadlinePropagationContext& dp_context,
                           std::string internal_message) {
  const auto& config = processor.GetHandler().GetConfig();
  dp_context.is_cancelled_by_deadline = true;
  processor.FinishProcessing();
  processor.SetForcedLogLevel(logging::Level::kWarning);

  auto& response = processor.GetRequest().GetHttpResponse();
  const auto status_code = config.deadline_expired_status_code;
  response.SetStatus(status_code);

  const http::CustomHandlerException exception_for_formatted_body{
      HandlerErrorCode::kClientError, config.deadline_expired_status_code,
      ExternalBody{"Deadline expired"},
      InternalMessage{std::move(internal_message)},
      ServiceErrorCode{"deadline_expired"}};
  SetFormattedErrorResponse(
      response, processor.GetHandler().GetFormattedExternalErrorBody(
                    exception_for_formatted_body));

  response.SetHeader(USERVER_NAMESPACE::http::headers::kXYaTaxiDeadlineExpired,
                     "1");
}

void SetUpInheritedDeadline(RequestProcessor& processor,
                            DeadlinePropagationContext& dp_context,
                            request::TaskInheritedData& inherited_data) {
  if (!processor.GetHandler().GetConfig().deadline_propagation_enabled) return;

  if (!processor.GetInitialDynamicConfig()[impl::kDeadlinePropagationEnabled]) {
    return;
  }

  const auto timeout = ParseTimeout(processor.GetRequest());
  if (!timeout) return;

  dp_context.need_log_response =
      processor.GetInitialDynamicConfig()[kLogRequest];

  auto& span = tracing::Span::CurrentSpan();
  span.AddNonInheritableTag("deadline_received_ms", timeout->count());

  const auto deadline = engine::Deadline::FromTimePoint(
      processor.GetRequest().GetStartTime() + *timeout);
  inherited_data.deadline = deadline;

  if (deadline.IsSurelyReachedApprox()) {
    HandleDeadlineExpired(processor, dp_context,
                          "Immediate timeout (deadline propagation)");
    return;
  }

  if (processor.GetInitialDynamicConfig()[kCancelHandleRequestByDeadline]) {
    engine::current_task::SetDeadline(deadline);
  }
}

void SetUpInheritedData(RequestProcessor& processor,
                        DeadlinePropagationContext& dp_context) {
  request::TaskInheritedData inherited_data{
      std::visit(
          utils::Overloaded{
              [](const std::string& path) -> std::string_view { return path; },
              [](const FallbackHandler&) -> std::string_view {
                return "<fallback>";
              }},
          processor.GetHandler().GetConfig().path),
      processor.GetRequest().GetMethodStr(),
      processor.GetRequest().GetStartTime(),
      engine::Deadline{},
  };

  const utils::FastScopeGuard set_inherited_data_guard([&]() noexcept {
    request::kTaskInheritedData.Set(std::move(inherited_data));
  });

  SetUpInheritedDeadline(processor, dp_context, inherited_data);
}

void CompleteDeadlinePropagation(RequestProcessor& processor,
                                 DeadlinePropagationContext& dp_context) {
  const auto& request = processor.GetRequest();
  auto& response = request.GetHttpResponse();

  const auto* const inherited_data = request::kTaskInheritedData.GetOptional();
  if (!inherited_data) {
    // Handling was interrupted before it got to SetUpInheritedData.
    return;
  }

  if (!inherited_data->deadline.IsReachable()) return;

  const bool cancelled_by_deadline =
      engine::current_task::CancellationReason() ==
          engine::TaskCancellationReason::kDeadline ||
      inherited_data->deadline_signal.IsExpired() ||
      inherited_data->deadline.IsReached();

  auto& span = tracing::Span::CurrentSpan();
  span.AddNonInheritableTag("cancelled_by_deadline", cancelled_by_deadline);

  if (cancelled_by_deadline && !dp_context.is_cancelled_by_deadline) {
    const auto& original_body = response.GetData();
    if (!original_body.empty() && span.ShouldLogDefault()) {
      span.AddNonInheritableTag("dp_original_body_size", original_body.size());
      if (dp_context.need_log_response) {
        span.AddNonInheritableTag(
            "dp_original_body",
            processor.GetHandler().GetResponseDataForLoggingChecked(
                request, processor.GetContext(), response.GetData()));
      }
    }
    HandleDeadlineExpired(processor, dp_context,
                          "Handling timeout (deadline propagation)");
  }
}

std::string CutTrailingSlash(
    std::string&& meta_type,
    server::handlers::UrlTrailingSlashOption trailing_slash) {
  if (trailing_slash == UrlTrailingSlashOption::kBoth && meta_type.size() > 1 &&
      meta_type.back() == '/') {
    meta_type.pop_back();
  }

  return std::move(meta_type);
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

std::unordered_map<int, logging::Level> ParseStatusCodesLogLevel(
    const std::unordered_map<std::string, std::string>& codes) {
  std::unordered_map<int, logging::Level> result;
  for (const auto& [key, value] : codes) {
    result[utils::FromString<int>(key)] = logging::LevelFromString(value);
  }
  return result;
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
      tracing_manager_(
          context.FindComponent<tracing::DefaultTracingManagerLocator>()
              .GetTracingManager()),
      log_level_for_status_codes_(ParseStatusCodesLogLevel(
          config["status-codes-log-level"]
              .As<std::unordered_map<std::string, std::string>>({}))),
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

  std::vector<utils::statistics::Label> labels{
      {"http_handler", config.Name()},
  };

  auto prefix = std::visit(
      utils::Overloaded{[&](const std::string& path) {
                          labels.emplace_back(
                              "http_path", utils::graphite::EscapeName(path));
                          return std::string{"http"};
                        },
                        [](FallbackHandler fallback) {
                          return "http.by-fallback." + ToString(fallback);
                        }},
      GetConfig().path);

  auto& statistics_storage =
      context.FindComponent<components::StatisticsStorage>().GetStorage();
  statistics_holder_ = statistics_storage.RegisterWriter(
      std::move(prefix),
      [this](utils::statistics::Writer& result) {
        FormatStatistics(result["handler"], *handler_statistics_);
        if constexpr (kIncludeServerHttpMetrics) {
          FormatStatistics(result["request"], *request_statistics_);
        }
      },
      std::move(labels));

  set_response_server_hostname_ =
      GetConfig().set_response_server_hostname.value_or(
          server_component.GetServer()
              .GetConfig()
              .set_response_server_hostname);
}

HttpHandlerBase::~HttpHandlerBase() { statistics_holder_.Unregister(); }

void HttpHandlerBase::HandleRequestStream(
    const http::HttpRequest& http_request,
    request::RequestContext& context) const {
  auto& response = http_request.GetHttpResponse();
  const utils::ScopeGuard scope([&response] { response.SetHeadersEnd(); });

  auto& http_response = http_request.GetHttpResponse();
  server::http::ResponseBodyStream response_body_stream{
      response.GetBodyProducer(), http_response};

  // Just in case HandleStreamRequest() throws an exception.
  // Though it can be changed in HandleStreamRequest().
  response_body_stream.SetStatusCode(500);

  try {
    HandleStreamRequest(http_request, context, response_body_stream);
  } catch (const CustomHandlerException& e) {
    response_body_stream.SetStatusCode(http::GetHttpStatus(e));

    for (const auto& [name, value] : e.GetExtraHeaders()) {
      response_body_stream.SetHeader(name, value);
    }
    if (e.IsExternalErrorBodyFormatted()) {
      response_body_stream.SetEndOfHeaders();
      response_body_stream.PushBodyChunk(std::string{e.GetExternalErrorBody()},
                                         engine::Deadline());
    } else {
      auto formatted_error = GetFormattedExternalErrorBody(e);
      SetFormattedErrorResponse(response_body_stream,
                                std::move(formatted_error), engine::Deadline());
    }
  } catch (const std::exception& e) {
    if (engine::current_task::ShouldCancel()) {
      LOG_WARNING() << "request task cancelled, exception in '" << HandlerName()
                    << "' handler in handle_request: " << e;
      response_body_stream.SetStatusCode(
          http::HttpStatus::kClientClosedRequest);
    } else {
      LOG_ERROR() << "exception in '" << HandlerName()
                  << "' handler in handle_request: " << e;
      response_body_stream.SetStatusCode(500);
      SetFormattedErrorResponse(response,
                                GetFormattedExternalErrorBody({
                                    HandlerErrorCode::kServerSideError,
                                    ExternalBody{response.GetData()},
                                }));
    }
  }
}

void HttpHandlerBase::HandleRequest(request::RequestBase& request,
                                    request::RequestContext& context) const {
  UASSERT(dynamic_cast<http::HttpRequestImpl*>(&request));
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
  auto& http_request_impl = static_cast<http::HttpRequestImpl&>(request);
  http::HttpRequest http_request(http_request_impl);
  auto& response = http_request.GetHttpResponse();
  std::optional<tracing::Span> span_storage;

  try {
    HttpHandlerStatisticsScope stats_scope(*handler_statistics_,
                                           http_request.GetMethod(), response);
    DeadlinePropagationContext dp_context;

    const auto meta_type = CutTrailingSlash(GetMetaType(http_request),
                                            GetConfig().url_trailing_slash);
    span_storage.emplace(MakeSpan(http_request, meta_type));
    // All fallible or logging steps should go after this to get good logs.

    RequestProcessor request_processor(*this, http_request_impl, http_request,
                                       context, config_source_.GetSnapshot());

    SetUpBaggage(http_request, request_processor.GetInitialDynamicConfig());
    LogYandexHeaders(http_request);

    request_processor.ProcessRequestStepNoScopeTime(
        "check_ratelimit",
        [this, &http_request] { CheckRatelimit(http_request); });

    request_processor.ProcessRequestStepNoScopeTime(
        "check_deadline_propagation", [&request_processor, &dp_context] {
          SetUpInheritedData(request_processor, dp_context);
        });

    request_processor.ProcessRequestStep(
        "http_check_auth",
        [this, &http_request, &context] { CheckAuth(http_request, context); });

    if (GetConfig().decompress_request) {
      request_processor.ProcessRequestStep(
          "http_decompress_request_body",
          [this, &http_request] { DecompressRequestBody(http_request); });
    }

    request_processor.ProcessRequestStep(
        "http_parse_request_data", [this, &http_request, &context] {
          ParseRequestData(http_request, context);
        });

    if (request_processor.GetInitialDynamicConfig()[kLogRequest]) {
      const bool need_log_request_headers =
          request_processor.GetInitialDynamicConfig()[kLogRequestHeaders];
      LOG_INFO() << "start handling"
                 << LogRequestExtra(
                        need_log_request_headers, http_request, meta_type,
                        GetRequestBodyForLoggingChecked(
                            http_request, context, http_request.RequestBody()),
                        http_request.RequestBody().length());
    }

    {
      // Don't hold the config snapshot for too long, especially with streaming.
      request_processor.ResetDynamicConfig();
    }

    request_processor.ProcessRequestStep(
        "http_handle_request", [this, &response, &http_request, &context] {
          if (response.IsBodyStreamed()) {
            HandleRequestStream(http_request, context);
          } else {
            // !IsBodyStreamed()
            response.SetData(HandleRequestThrow(http_request, context));
          }
        });

    CompleteDeadlinePropagation(request_processor, dp_context);
    if (GetConfig().set_tracing_headers) {
      tracing_manager_.FillResponseWithTracingContext(*span_storage, response);
    }
    if (dp_context.is_cancelled_by_deadline) {
      stats_scope.OnCancelledByDeadline();
    }
  } catch (const std::exception& ex) {
    LOG_ERROR() << "unable to handle request: " << ex;
  }

  SetResponseAcceptEncoding(response);
  SetResponseServerHostname(response);
  response.SetHeadersEnd();
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
    server::http::ResponseBodyStream&) const {
  throw std::runtime_error(
      "stream HandleStreamRequest() is executed, but the handler doesn't "
      "override HandleStreamRequest().");
}

void HttpHandlerBase::ReportMalformedRequest(
    request::RequestBase& request) const {
  try {
    UASSERT(dynamic_cast<http::HttpRequestImpl*>(&request));
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
    auto& http_request_impl = static_cast<http::HttpRequestImpl&>(request);
    const http::HttpRequest http_request(http_request_impl);
    auto& response = http_request.GetHttpResponse();

    SetFormattedErrorResponse(
        response,
        GetFormattedExternalErrorBody({HandlerErrorCode::kRequestParseError,
                                       ExternalBody{response.GetData()}}));
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

HttpHandlerStatistics& HttpHandlerBase::GetHandlerStatistics() const {
  return *handler_statistics_;
}

HttpRequestStatistics& HttpHandlerBase::GetRequestStatistics() const {
  return *request_statistics_;
}

logging::Level HttpHandlerBase::GetLogLevelForResponseStatus(
    http::HttpStatus status) const {
  const auto status_code = static_cast<int>(status);
  const auto* const level =
      utils::FindOrNullptr(log_level_for_status_codes_, status_code);
  if (level) return *level;

  if (status_code >= 400 && status_code <= 499) return logging::Level::kWarning;
  if (status_code >= 500 && status_code <= 599) return logging::Level::kError;
  return logging::Level::kInfo;
}

FormattedErrorData HttpHandlerBase::GetFormattedExternalErrorBody(
    const CustomHandlerException& exc) const {
  return {exc.GetExternalErrorBody()};
}

tracing::Span HttpHandlerBase::MakeSpan(const http::HttpRequest& http_request,
                                        const std::string& meta_type) const {
  tracing::SpanBuilder span_builder(fmt::format("http/{}", HandlerName()));
  tracing_manager_.TryFillSpanBuilderFromRequest(http_request, span_builder);
  auto span = std::move(span_builder).Build();

  span.SetLocalLogLevel(log_level_);

  span.AddNonInheritableTag(tracing::kHttpMetaType, meta_type);
  span.AddNonInheritableTag(tracing::kType, kTracingTypeResponse);
  span.AddNonInheritableTag(tracing::kHttpMethod, http_request.GetMethodStr());

  return span;
}

void HttpHandlerBase::CheckAuth(const http::HttpRequest& http_request,
                                request::RequestContext& context) const {
  if (!NeedCheckAuth()) {
    LOG_DEBUG() << "auth checks are disabled for current handler";
    return;
  }

  auth::CheckAuth(auth_checkers_, http_request, context);
}

void HttpHandlerBase::CheckRatelimit(
    const http::HttpRequest& http_request) const {
  auto& statistics = handler_statistics_->ForMethod(http_request.GetMethod());

  const bool success = rate_limit_.Obtain();
  if (!success) {
    UASSERT(GetConfig().max_requests_per_second);

    auto& http_response = http_request.GetHttpResponse();
    auto log_reason =
        fmt::format("reached max_requests_per_second={}",
                    GetConfig().max_requests_per_second.value_or(0));
    SetThrottleReason(
        http_response, std::move(log_reason),
        std::string{
            USERVER_NAMESPACE::http::headers::ratelimit_reason::kGlobal});
    statistics.IncrementRateLimitReached();

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
        std::string{
            USERVER_NAMESPACE::http::headers::ratelimit_reason::kInFlight});

    statistics.IncrementTooManyRequestsInFlight();

    throw ExceptionWithCode<HandlerErrorCode::kTooManyRequests>();
  }
}

void HttpHandlerBase::DecompressRequestBody(
    http::HttpRequest& http_request) const {
  if (!http_request.IsBodyCompressed()) return;

  const auto& content_encoding = http_request.GetHeader(
      USERVER_NAMESPACE::http::headers::kContentEncoding);
  const utils::FastScopeGuard encoding_remove_guard{[&http_request]() noexcept {
    http_request.RemoveHeader(
        USERVER_NAMESPACE::http::headers::kContentEncoding);
  }};

  try {
    if (content_encoding == "gzip") {
      http_request.RemoveHeader("Content-Encoding");
      auto body = compression::gzip::Decompress(
          http_request.RequestBody(),
          GetConfig().request_config.max_request_size);
      http_request.SetRequestBody(std::move(body));
      if (GetConfig().request_config.parse_args_from_body) {
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
  const std::size_t limit = GetConfig().request_body_size_log_limit;
  return utils::log::ToLimitedUtf8(request_body, limit);
}

std::string HttpHandlerBase::GetResponseDataForLogging(
    const http::HttpRequest&, request::RequestContext&,
    const std::string& response_data) const {
  const std::size_t limit = GetConfig().response_data_size_log_limit;
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
void HttpHandlerBase::FormatStatistics(utils::statistics::Writer result,
                                       const HttpStatistics& stats) {
  using Snapshot = typename HttpStatistics::Snapshot;
  Snapshot total;

  for (const auto method : GetAllowedMethods()) {
    const Snapshot by_method{stats.GetByMethod(method)};
    if (IsMethodStatisticIncluded()) {
      result.ValueWithLabels(by_method, {"http_method", ToString(method)});
    }
    total.Add(by_method);
  }

  result = total;
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
    status-codes-log-level:
        type: object
        properties: {}
        additionalProperties:
            type: string
            description: log level
        description: HTTP status code -> log level map
)");
}

}  // namespace server::handlers

USERVER_NAMESPACE_END
