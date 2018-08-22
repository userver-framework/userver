#pragma once

#include <atomic>
#include <functional>
#include <memory>

#include <engine/async_task.hpp>
#include <server/handlers/handler_base.hpp>

#include <server/request/request_base.hpp>

namespace server {

class TaskProcessor;

namespace request {

class RequestTask {
 public:
  using NotifyCb = std::function<void()>;

  RequestTask(engine::TaskProcessor* task_processor,
              const handlers::HandlerBase* handler,
              std::unique_ptr<RequestBase>&& request, NotifyCb&& notify_cb);

  bool IsComplete() const { return is_complete_; }

  void Start(bool can_fail);
  void SetComplete();

  RequestBase& GetRequest() const { return *request_; }
  const handlers::HandlerBase* GetHandler() const { return handler_; }

 private:
  engine::TaskProcessor* task_processor_;
  const handlers::HandlerBase* handler_;
  std::unique_ptr<RequestBase> request_;
  NotifyCb notify_cb_;
  std::atomic<bool> is_complete_;
  engine::AsyncTask<void> async_task_;
};

}  // namespace request
}  // namespace server
