#include "http_request_handler.hpp"

#include <stdexcept>

#include <logging/logger.hpp>

#include <server/http/http_request.hpp>
#include <server/http/http_request_impl.hpp>
#include <server/http/http_response.hpp>

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
    std::unique_ptr<request::RequestBase>&& request,
    std::function<void()>&& notify_func) const {
  auto& http_request = dynamic_cast<HttpRequestImpl&>(*request);
  HandlerInfo handler_info;
  if (GetHandlerInfo(http_request.GetRequestPath(), handler_info))
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
    task.SetComplete();
    request.SetResponseNotifyTime();
    request.SetCompleteNotifyTime();
    response.SetReady();
  }
}

void HttpRequestHandler::DisableAddHandler() { add_handler_disabled_ = true; }

bool HttpRequestHandler::AddHandler(
    const handlers::HandlerBase& handler,
    const components::ComponentContext& component_context) {
  if (add_handler_disabled_) {
    LOG_ERROR() << "handler adding disabled";
    return false;
  }
  std::lock_guard<engine::Mutex> lock(handler_infos_mutex_);
  if (is_monitor_ != handler.IsMonitor()) {
    LOG_ERROR() << "adding " << (handler.IsMonitor() ? "" : "non-")
                << "monitor handler to " << (is_monitor_ ? "" : "non-")
                << "monitor HttpRequestHandler";
    return false;
  }
  engine::TaskProcessor* task_processor =
      component_context.GetTaskProcessor(handler.GetConfig().task_processor);
  if (task_processor == nullptr) {
    LOG_ERROR() << "can't find task_processor with name '"
                << handler.GetConfig().task_processor << '\'';
    return false;
  }
  return handler_infos_
      .emplace(std::piecewise_construct, std::tie(handler.GetConfig().path),
               std::tie(*task_processor, handler))
      .second;
}

bool HttpRequestHandler::GetHandlerInfo(
    const std::string& path,
    HttpRequestHandler::HandlerInfo& handler_info) const {
  if (!add_handler_disabled_) {
    LOG_ERROR()
        << "handler adding must be disabled before GetHandlerInfo() call";
    return false;
  }
  auto it = handler_infos_.find(path);
  if (it == handler_infos_.end()) {
    std::string path_prefix = path;
    for (size_t pos = path_prefix.rfind('/'); pos != std::string::npos;
         pos = path_prefix.rfind('/')) {
      path_prefix.resize(pos + 1);
      path_prefix += '*';
      it = handler_infos_.find(path_prefix);
      if (it != handler_infos_.end()) {
        handler_info = it->second;
        handler_info.matched_path_length = pos + 1;
        return true;
      }
      path_prefix.resize(pos);  // remove trailing "/*"
    }
    return false;
  }
  handler_info = it->second;
  handler_info.matched_path_length = path.size();
  return true;
}

}  // namespace http
}  // namespace server
