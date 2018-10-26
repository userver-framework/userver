#include "http_request_handler.hpp"

#include <stdexcept>

#include <logging/logger.hpp>
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

std::shared_ptr<request::RequestTask> HttpRequestHandler::PrepareRequestTask(
    std::shared_ptr<request::RequestBase>&& request,
    std::function<void()>&& notify_func) const {
  auto& http_request = dynamic_cast<http::HttpRequestImpl&>(*request);
  if (new_request_hook_) new_request_hook_(request);

  auto handler_info =
      GetHandlerInfo(http_request.GetMethod(), http_request.GetRequestPath());
  http_request.SetMatchedPathLength(handler_info.matched_path_length);
  // assert(handler_info.task_processor); -- false if handler not found
  auto task = std::make_shared<request::RequestTask>(
      handler_info.task_processor, handler_info.handler, std::move(request),
      std::move(notify_func));
  task->GetRequest().SetTaskCreateTime();
  return task;
}

void HttpRequestHandler::ProcessRequest(request::RequestTask& task) const {
  auto& request = dynamic_cast<HttpRequestImpl&>(task.GetRequest());
  auto& response = request.GetHttpResponse();

  if (response.GetStatus() == HttpStatus::kOk) {
    task.Start(!is_monitor_);
  } else {
    request.SetResponseNotifyTime();
    request.SetCompleteNotifyTime();
    response.SetReady();
    task.SetComplete();
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
