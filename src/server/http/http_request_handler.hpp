#pragma once

#include <boost/optional.hpp>

#include <components/component_context.hpp>
#include <engine/task/task_processor.hpp>
#include <server/handlers/handler_base.hpp>
#include <server/request/request_base.hpp>
#include <server/request/request_handler_base.hpp>
#include <server/request/request_task.hpp>

namespace server {
namespace http {

class HttpRequestHandler : public request::RequestHandlerBase {
 public:
  struct HandlerInfo {
    HandlerInfo() = default;
    HandlerInfo(engine::TaskProcessor& task_processor,
                const handlers::HandlerBase& handler)
        : task_processor(&task_processor), handler(&handler) {}

    engine::TaskProcessor* task_processor = nullptr;
    const handlers::HandlerBase* handler = nullptr;
    size_t matched_path_length = 0;
  };

  HttpRequestHandler(
      const components::ComponentContext& component_context,
      const boost::optional<std::string>& logger_access_component,
      const boost::optional<std::string>& logger_access_tskv_component,
      bool is_monitor);

  std::unique_ptr<request::RequestTask> PrepareRequestTask(
      std::unique_ptr<request::RequestBase>&& request,
      std::function<void()>&& notify_func) const override;
  void ProcessRequest(request::RequestTask& task) const override;

  bool GetHandlerInfo(const std::string& path, HandlerInfo& handler_info) const;

 private:
  std::unordered_map<std::string, HandlerInfo> handler_infos_;
  bool is_monitor_;
};

}  // namespace http
}  // namespace server
