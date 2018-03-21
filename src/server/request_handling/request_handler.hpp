#pragma once

#include <boost/optional.hpp>

#include <components/component_context.hpp>
#include <engine/task/task_processor.hpp>
#include <logging/logger.hpp>
#include <server/components/handlers/handler_base.hpp>
#include <server/request/request_base.hpp>

#include "request_task.hpp"

namespace server {
namespace request_handling {

class RequestHandler {
 public:
  struct HandlerInfo {
    HandlerInfo() = default;
    HandlerInfo(engine::TaskProcessor& task_processor,
                const components::handlers::HandlerBase& handler)
        : task_processor(&task_processor), handler(&handler) {}

    engine::TaskProcessor* task_processor = nullptr;
    const components::handlers::HandlerBase* handler = nullptr;
    size_t matched_path_length = 0;
  };

  RequestHandler(
      const components::ComponentContext& component_context,
      const boost::optional<std::string>& logger_access_component,
      const boost::optional<std::string>& logger_access_tskv_component);

  std::unique_ptr<RequestTask> PrepareRequestTask(
      std::unique_ptr<request::RequestBase>&& request,
      std::function<void()>&& notify_func, bool monitor);
  void ProcessRequest(RequestTask& task, bool monitor);
  const components::ComponentContext& GetComponentContext() const {
    return component_context_;
  }

  const logging::LoggerPtr& LoggerAccess() const { return logger_access_; }
  const logging::LoggerPtr& LoggerAccessTskv() const {
    return logger_access_tskv_;
  }

 private:
  bool GetHandlerInfo(const std::string& path, HandlerInfo& handler_info) const;

  const components::ComponentContext& component_context_;
  logging::LoggerPtr logger_access_;
  logging::LoggerPtr logger_access_tskv_;
  std::unordered_map<std::string, HandlerInfo> handler_infos_;
};

}  // namespace request_handling
}  // namespace server
