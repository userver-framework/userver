#pragma once

#include <boost/optional.hpp>

#include <components/component_context.hpp>
#include <engine/mutex.hpp>
#include <server/handlers/handler_base.hpp>
#include <server/request/request_base.hpp>

#include <engine/task/task_processor.hpp>
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

  std::shared_ptr<request::RequestTask> PrepareRequestTask(
      std::unique_ptr<request::RequestBase>&& request,
      std::function<void()>&& notify_func) const override;
  void ProcessRequest(request::RequestTask& task) const override;

  void DisableAddHandler();
  __attribute__((warn_unused_result)) bool AddHandler(
      const handlers::HandlerBase& handler,
      const components::ComponentContext& component_context);
  bool GetHandlerInfo(const std::string& path, HandlerInfo& handler_info) const;

 private:
  // handler_infos_mutex_ is used for pushing handlers into handler_infos_
  // before server start. After start handler_infos_ is read only and
  // synchronization is not needed.
  engine::Mutex handler_infos_mutex_;
  std::unordered_map<std::string, HandlerInfo> handler_infos_;

  std::atomic<bool> add_handler_disabled_;
  bool is_monitor_;
};

}  // namespace http
}  // namespace server
