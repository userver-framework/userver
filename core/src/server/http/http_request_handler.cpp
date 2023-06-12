#include "http_request_handler.hpp"

#include <chrono>
#include <stdexcept>

#include <server/handlers/http_handler_base_statistics.hpp>
#include <server/request/task_inherited_request_impl.hpp>
#include <userver/components/statistics_storage.hpp>
#include <userver/dynamic_config/storage/component.hpp>
#include <userver/dynamic_config/value.hpp>
#include <userver/engine/async.hpp>
#include <userver/http/common_headers.hpp>
#include <userver/logging/component.hpp>
#include <userver/logging/logger.hpp>
#include <userver/server/http/http_request.hpp>
#include <userver/server/http/http_response.hpp>
#include <userver/server/request/task_inherited_request.hpp>
#include <userver/utils/assert.hpp>
#include "http_request_impl.hpp"

USERVER_NAMESPACE_BEGIN

namespace server::http {
namespace {

engine::TaskWithResult<void> StartFailsafeTask(
    std::shared_ptr<request::RequestBase> request) {
  UASSERT(dynamic_cast<http::HttpRequestImpl*>(&*request));
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
  auto& http_request = static_cast<http::HttpRequestImpl&>(*request);

  const auto* handler = http_request.GetHttpHandler();
  static handlers::HttpRequestStatistics dummy_statistics;

  http_request.SetHttpHandlerStatistics(dummy_statistics);

  return engine::AsyncNoSpan([request = std::move(request), handler]() {
    request->SetTaskStartTime();
    if (handler) handler->ReportMalformedRequest(*request);
    request->SetResponseNotifyTime();
    request->GetResponse().SetReady();
  });
}

}  // namespace

HttpRequestHandler::HttpRequestHandler(
    const components::ComponentContext& component_context,
    const std::optional<std::string>& logger_access_component,
    const std::optional<std::string>& logger_access_tskv_component,
    bool is_monitor, std::string server_name)
    : add_handler_disabled_(false),
      is_monitor_(is_monitor),
      server_name_(std::move(server_name)),
      rate_limit_(utils::TokenBucket::MakeUnbounded()),
      metrics_(component_context.FindComponent<components::StatisticsStorage>()
                   .GetMetricsStorage()),
      config_source_(
          component_context.FindComponent<components::DynamicConfig>()
              .GetSource()) {
  auto& logging_component =
      component_context.FindComponent<components::Logging>();

  if (logger_access_component && !logger_access_component->empty()) {
    logger_access_ = logging_component.GetLogger(*logger_access_component);
  } else {
    LOG_INFO() << "Access log is disabled";
  }

  if (logger_access_tskv_component && !logger_access_tskv_component->empty()) {
    logger_access_tskv_ =
        logging_component.GetLogger(*logger_access_tskv_component);
  } else {
    LOG_INFO() << "Access_tskv log is disabled";
  }
}

namespace {

struct CcCustomStatus final {
  HttpStatus initial_status_code;
  std::chrono::milliseconds max_time_delta;
};

CcCustomStatus ParseRuntimeCfg(const dynamic_config::DocsMap& docs_map) {
  auto obj = docs_map.Get("USERVER_RPS_CCONTROL_CUSTOM_STATUS");
  return CcCustomStatus{
      static_cast<HttpStatus>(obj["initial-status-code"].As<int>(429)),
      std::chrono::milliseconds{obj["max-time-ms"].As<size_t>(10000)}};
}

constexpr dynamic_config::Key<ParseRuntimeCfg> kCcCustomStatus{};

bool ParseStreamApiEnabled(const dynamic_config::DocsMap& docs_map) {
  return docs_map.Get("USERVER_HANDLER_STREAM_API_ENABLED").As<bool>();
}

constexpr dynamic_config::Key<ParseStreamApiEnabled> kStreamApiEnabled{};

utils::statistics::MetricTag<std::atomic<size_t>> kCcStatusCodeIsCustom{
    "congestion-control.rps.is-custom-status-activated"};

}  // namespace

engine::TaskWithResult<void> HttpRequestHandler::StartRequestTask(
    std::shared_ptr<request::RequestBase> request) const {
  UASSERT(dynamic_cast<http::HttpRequestImpl*>(&*request));
  const auto& http_request =
      // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
      static_cast<const http::HttpRequestImpl&>(*request);

  auto& http_response = http_request.GetHttpResponse();
  http_response.SetHeader(USERVER_NAMESPACE::http::headers::kServer,
                          server_name_);
  if (http_response.IsReady()) {
    // Request is broken somehow, user handler must not be called
    request->SetTaskCreateTime();
    return StartFailsafeTask(std::move(request));
  }

  if (new_request_hook_) new_request_hook_(request);

  request->SetTaskCreateTime();

  auto* task_processor = http_request.GetTaskProcessor();
  const auto* handler = http_request.GetHttpHandler();
  if (!task_processor || !handler) {
    // No handler found, response status is already set
    // by HttpRequestConstructor::CheckStatus
    return StartFailsafeTask(std::move(request));
  }
  auto throttling_enabled = handler->GetConfig().throttling_enabled;

  if (throttling_enabled && http_response.IsLimitReached()) {
    SetThrottleReason(http_response, "Too many pending responses",
                      std::string{USERVER_NAMESPACE::http::headers::
                                      ratelimit_reason::kMaxPendingResponses});

    http_request.SetResponseStatus(HttpStatus::kTooManyRequests);
    http_request.GetHttpResponse().SetReady();
    request->SetTaskCreateTime();
    LOG_LIMITED_ERROR() << "Request throttled (too many pending responses, "
                           "limit via 'server.max_response_size_in_flight')";
    return StartFailsafeTask(std::move(request));
  }
  const auto& config = config_source_.GetSnapshot();

  if (throttling_enabled && !rate_limit_.Obtain()) {
    auto config_var = config[kCcCustomStatus];
    const auto& delta = config_var.max_time_delta;

    auto status = HttpStatus::kTooManyRequests;
    if (cc_enabled_tp_ > std::chrono::steady_clock::now() - delta) {
      status = config_var.initial_status_code;
      metrics_->GetMetric(kCcStatusCodeIsCustom) = 1;
    } else {
      status = cc_status_code_.load();
      metrics_->GetMetric(kCcStatusCodeIsCustom) = 0;
    }

    SetThrottleReason(
        http_response, "congestion-control",
        std::string{USERVER_NAMESPACE::http::headers::ratelimit_reason::kCC});

    http_response.SetStatus(status);
    http_response.SetReady();

    LOG_LIMITED_ERROR()
        << "Request throttled (congestion control, "
           "limit via USERVER_RPS_CCONTROL and USERVER_RPS_CCONTROL_ENABLED), "
        << "limit=" << rate_limit_.GetRatePs() << "/sec, "
        << "url=" << http_request.GetUrl()
        << ", status_code=" << static_cast<size_t>(status);

    return StartFailsafeTask(std::move(request));
  }

  if (handler->GetConfig().response_body_stream && config[kStreamApiEnabled]) {
    http_response.SetStreamBody();
  }

  auto payload = [request = std::move(request), handler] {
    server::request::kTaskInheritedRequest.Set(
        std::static_pointer_cast<HttpRequestImpl>(request));

    request->SetTaskStartTime();

    request::RequestContext context;
    handler->HandleRequest(*request, context);

    const auto now = std::chrono::steady_clock::now();
    request->SetResponseNotifyTime(now);
    request->GetResponse().SetReady(now);
  };

  if (!is_monitor_ && throttling_enabled) {
    return engine::AsyncNoSpan(*task_processor, std::move(payload));
  } else {
    return engine::CriticalAsyncNoSpan(*task_processor, std::move(payload));
  }
}  // namespace http

void HttpRequestHandler::DisableAddHandler() {
  const auto was_enabled = !add_handler_disabled_.exchange(true);
  UASSERT(was_enabled);
}

void HttpRequestHandler::AddHandler(const handlers::HttpHandlerBase& handler,
                                    engine::TaskProcessor& task_processor) {
  UASSERT_MSG(!add_handler_disabled_, "handler adding disabled");
  if (is_monitor_ != handler.IsMonitor()) {
    throw std::runtime_error(
        std::string("adding ") + (handler.IsMonitor() ? "" : "non-") +
        "monitor handler to " + (is_monitor_ ? "" : "non-") +
        "monitor HttpRequestHandler");
  }
  std::lock_guard<engine::Mutex> lock(handler_infos_mutex_);
  handler_info_index_.AddHandler(handler, task_processor);
}

bool HttpRequestHandler::IsAddHandlerDisabled() const noexcept {
  return add_handler_disabled_.load();
}

const HandlerInfoIndex& HttpRequestHandler::GetHandlerInfoIndex() const {
  UASSERT_MSG(
      add_handler_disabled_,
      "handler adding must be disabled before GetHandlerInfoIndex() call");
  return handler_info_index_;
}

void HttpRequestHandler::SetNewRequestHook(NewRequestHook hook) {
  new_request_hook_ = std::move(hook);
}

void HttpRequestHandler::SetRpsRatelimit(std::optional<size_t> rps) {
  if (rps) {
    if (rate_limit_.IsUnbounded()) {
      cc_enabled_tp_ = std::chrono::steady_clock::now();
      metrics_->GetMetric(kCcStatusCodeIsCustom) = 0;
    }

    const auto rps_val = *rps;
    if (rps_val > 0) {
      rate_limit_.SetMaxSize(rps_val);
      rate_limit_.SetRefillPolicy(
          {1, utils::TokenBucket::Duration{std::chrono::seconds(1)} / rps_val});
    } else {
      rate_limit_.SetMaxSize(0);
    }
  } else {
    rate_limit_.SetMaxSize(1);  // in case it was zero
    rate_limit_.SetInstantRefillPolicy();
  }
}

void HttpRequestHandler::SetRpsRatelimitStatusCode(HttpStatus status_code) {
  LOG_DEBUG() << "CC status code changed to " << static_cast<int>(status_code);
  cc_status_code_ = status_code;
}

}  // namespace server::http

USERVER_NAMESPACE_END
