#include "http_request_handler.hpp"

#include <stdexcept>

#include <engine/async.hpp>
#include <logging/logger.hpp>
#include <server/handlers/http_handler_base_statistics.hpp>
#include <server/http/http_request.hpp>
#include <server/http/http_response.hpp>
#include "http_request_impl.hpp"

namespace server {
namespace http {

HttpRequestHandler::HttpRequestHandler(
    const components::ComponentContext& component_context,
    const std::optional<std::string>& logger_access_component,
    const std::optional<std::string>& logger_access_tskv_component,
    bool is_monitor)
    : add_handler_disabled_(false),
      is_monitor_(is_monitor),
      rate_limit_(1, std::chrono::seconds(0)) {
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

engine::TaskWithResult<void> HttpRequestHandler::StartRequestTask(
    std::shared_ptr<request::RequestBase> request) const {
  const auto& http_request =
      dynamic_cast<const http::HttpRequestImpl&>(*request);
  LOG_TRACE() << "ready=" << http_request.GetResponse().IsReady();
  if (http_request.GetResponse().IsReady()) {
    // Request is broken somehow, user handler must not be called
    request->SetTaskCreateTime();
    return StartFailsafeTask(std::move(request));
  }

  if (http_request.GetResponse().IsLimitReached()) {
    http_request.SetResponseStatus(HttpStatus::kTooManyRequests);
    http_request.GetHttpResponse().SetReady();
    request->SetTaskCreateTime();
    return StartFailsafeTask(std::move(request));
  }

  if (new_request_hook_) new_request_hook_(request);

  request->SetTaskCreateTime();

  auto* task_processor = http_request.GetTaskProcessor();
  auto* handler = http_request.GetHttpHandler();
  if (!task_processor || !handler) {
    // No handler found, response status is already set
    // by HttpRequestConstructor::CheckStatus
    return StartFailsafeTask(std::move(request));
  }
  if (!rate_limit_.Obtain()) {
    http_request.SetResponseStatus(HttpStatus::kTooManyRequests);
    http_request.GetHttpResponse().SetReady();
    LOG_ERROR() << "Request throttled (congestion control)";
    return StartFailsafeTask(std::move(request));
  }

  auto payload = [request = std::move(request), handler] {
    request->SetTaskStartTime();

    request::RequestContext context;
    handler->HandleRequest(*request, context);

    request->SetResponseNotifyTime();
    request->GetResponse().SetReady();
  };

  if (!is_monitor_) {
    return engine::impl::Async(*task_processor, std::move(payload));
  } else {
    return engine::impl::CriticalAsync(*task_processor, std::move(payload));
  }
}  // namespace http

void HttpRequestHandler::DisableAddHandler() { add_handler_disabled_ = true; }

void HttpRequestHandler::AddHandler(const handlers::HttpHandlerBase& handler,
                                    engine::TaskProcessor& task_processor) {
  if (add_handler_disabled_) {
    throw std::runtime_error("handler adding disabled");
  }
  if (is_monitor_ != handler.IsMonitor()) {
    throw std::runtime_error(
        std::string("adding ") + (handler.IsMonitor() ? "" : "non-") +
        "monitor handler to " + (is_monitor_ ? "" : "non-") +
        "monitor HttpRequestHandler");
  }
  std::lock_guard<engine::Mutex> lock(handler_infos_mutex_);
  handler_info_index_.AddHandler(handler, task_processor);
}

const HandlerInfoIndex& HttpRequestHandler::GetHandlerInfoIndex() const {
  if (!add_handler_disabled_) {
    throw std::runtime_error(
        "handler adding must be disabled before GetHandlerInfoIndex() call");
  }
  return handler_info_index_;
}

void HttpRequestHandler::SetNewRequestHook(NewRequestHook hook) {
  new_request_hook_ = std::move(hook);
}

engine::TaskWithResult<void> HttpRequestHandler::StartFailsafeTask(
    std::shared_ptr<request::RequestBase> request) const {
  auto& http_request = dynamic_cast<http::HttpRequestImpl&>(*request);
  auto* handler = http_request.GetHttpHandler();
  static handlers::HttpHandlerStatistics dummy_statistics;

  http_request.SetHttpHandlerStatistics(dummy_statistics);

  return engine::impl::Async([request = std::move(request), handler]() {
    request->SetTaskStartTime();
    if (handler) handler->ReportMalformedRequest(*request);
    request->SetResponseNotifyTime();
    request->GetResponse().SetReady();
  });
}

void HttpRequestHandler::SetRpsRatelimit(std::optional<size_t> rps) {
  if (rps) {
    rate_limit_.SetMaxSize(*rps);
    rate_limit_.SetUpdateInterval(
        utils::TokenBucket::Duration{std::chrono::seconds(1)} / *rps);
  } else {
    rate_limit_.SetUpdateInterval(utils::TokenBucket::Duration(0));
  }
}

}  // namespace http
}  // namespace server
