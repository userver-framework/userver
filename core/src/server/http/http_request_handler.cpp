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
    const boost::optional<std::string>& logger_access_component,
    const boost::optional<std::string>& logger_access_tskv_component,
    bool is_monitor)
    : request::RequestHandlerBase(component_context, logger_access_component,
                                  logger_access_tskv_component),
      add_handler_disabled_(false),
      is_monitor_(is_monitor) {}

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

  if (new_request_hook_) new_request_hook_(request);

  request->SetTaskCreateTime();

  auto* task_processor = http_request.GetTaskProcessor();
  auto* handler = http_request.GetHttpHandler();
  if (!task_processor || !handler) {
    // No handler found, response status is already set
    // by HttpRequestConstructor::CheckStatus
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
}

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

}  // namespace http
}  // namespace server
