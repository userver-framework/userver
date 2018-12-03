#include "http_request_handler.hpp"

#include <stdexcept>

#include <engine/async.hpp>
#include <logging/logger.hpp>
#include <server/http/http_request.hpp>
#include <server/http/http_response.hpp>

#include "http_request_impl.hpp"

namespace server {
namespace http {

namespace {
const std::string kHttpRequestSpanTag = "http_request";
}  // namespace

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
  auto& http_request = dynamic_cast<http::HttpRequestImpl&>(*request);
  if (new_request_hook_) new_request_hook_(request);

  auto handler_info =
      GetHandlerInfo(http_request.GetMethod(), http_request.GetRequestPath());
  request->SetTaskCreateTime();

  if (!handler_info.task_processor) {
    // No handler found, response status is already set
    // by HttpRequestConstructor::CheckStatus

    return engine::Async([request = std::move(request)]() {
      request->SetTaskStartTime();
      request->SetResponseNotifyTime();
      request->SetCompleteNotifyTime();
      request->GetResponse().SetReady();
    });
  }

  http_request.SetMatchedPathLength(handler_info.matched_path_length);

  auto payload = [
    request = std::move(request), handler = std::move(handler_info.handler)
  ] {
    request->SetTaskStartTime();

    tracing::Span span(kHttpRequestSpanTag);
    request::RequestContext context;
    handler->HandleRequest(*request, context);

    request->SetResponseNotifyTime();
    request->GetResponse().SetReady();
    handler->OnRequestComplete(*request, context);
    request->SetCompleteNotifyTime();
  };

  if (!is_monitor_) {
    return engine::Async(*handler_info.task_processor, std::move(payload));
  } else {
    return engine::CriticalAsync(*handler_info.task_processor,
                                 std::move(payload));
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

HandlerInfo HttpRequestHandler::GetHandlerInfo(HttpMethod method,
                                               const std::string& path) const {
  if (!add_handler_disabled_) {
    throw std::runtime_error(
        "handler adding must be disabled before GetHandlerInfo() call");
  }
  return handler_info_index_.GetHandlerInfo(method, path);
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

}  // namespace http
}  // namespace server
