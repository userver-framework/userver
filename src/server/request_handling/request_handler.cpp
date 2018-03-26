#include "request_handler.hpp"

#include <stdexcept>

#include <logging/component.hpp>
#include <server/request/monitor.hpp>
#include <server/request/response_base.hpp>

namespace server {
namespace request_handling {

RequestHandler::RequestHandler(
    const components::ComponentContext& component_context,
    const boost::optional<std::string>& logger_access_component,
    const boost::optional<std::string>& logger_access_tskv_component)
    : component_context_(component_context) {
  for (const auto& component : component_context) {
    if (auto handler = dynamic_cast<const handlers::HandlerBase*>(
            component.second.get())) {
      engine::TaskProcessor* task_processor =
          component_context.GetTaskProcessor(
              handler->GetConfig().task_processor);
      if (task_processor == nullptr) {
        throw std::runtime_error("can't find task_processor with name '" +
                                 handler->GetConfig().task_processor + '\'');
      }
      handler_infos_.emplace(std::piecewise_construct,
                             std::tie(handler->GetConfig().path),
                             std::tie(*task_processor, *handler));
    }
  }

  auto* logging_component =
      component_context.FindComponent<components::Logging>(
          components::Logging::kName);

  if (logger_access_component && !logger_access_component->empty()) {
    if (logging_component) {
      logger_access_ = logging_component->GetLogger(*logger_access_component);
    } else {
      throw std::runtime_error(
          "can't find logger_access component with name '" +
          *logger_access_component + '\'');
    }
  }

  if (logger_access_tskv_component && !logger_access_tskv_component->empty()) {
    if (logging_component) {
      logger_access_tskv_ =
          logging_component->GetLogger(*logger_access_tskv_component);
    } else {
      throw std::runtime_error(
          "can't find logger_access_tskv component with name '" +
          *logger_access_tskv_component + '\'');
    }
  }
}

std::unique_ptr<RequestTask> RequestHandler::PrepareRequestTask(
    std::unique_ptr<request::RequestBase>&& request,
    std::function<void()>&& notify_func, bool monitor) {
  HandlerInfo handler_info;
  if (!monitor && GetHandlerInfo(request->GetRequestPath(), handler_info))
    request->SetMatchedPathLength(handler_info.matched_path_length);
  std::unique_ptr<RequestTask> task = std::make_unique<RequestTask>(
      handler_info.task_processor, handler_info.handler, std::move(request),
      std::move(notify_func));
  task->GetRequest().SetTaskCreateTime();
  return task;
}

void RequestHandler::ProcessRequest(RequestTask& task, bool monitor) {
  if (monitor) {
    task.GetRequest().GetResponse().SetStatusOk();
    task.GetRequest().GetResponse().SetData(
        component_context_.GetServerMonitor().GetJsonData(task.GetRequest()));
    task.GetRequest().SetResponseNotifyTime();
    task.GetRequest().SetCompleteNotifyTime();
    task.GetRequest().GetResponse().SetReady();
    task.SetComplete();
  } else if (task.GetHandler()) {
    if (!task.GetTaskProcessor().AddTask(&task, true))
      LOG_WARNING() << "failed to add task";
  } else {
    task.GetRequest().GetResponse().SetStatusNotFound();
    task.GetRequest().SetResponseNotifyTime();
    task.GetRequest().SetCompleteNotifyTime();
    task.GetRequest().GetResponse().SetReady();
    task.SetComplete();
  }
}

bool RequestHandler::GetHandlerInfo(
    const std::string& path, RequestHandler::HandlerInfo& handler_info) const {
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

}  // namespace request_handling
}  // namespace server
