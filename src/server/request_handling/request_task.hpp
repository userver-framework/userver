#pragma once

#include <functional>
#include <memory>

#include <engine/task/task.hpp>
#include <server/components/handlers/handler_base.hpp>
#include <server/request/request_base.hpp>

namespace server {
namespace request_handling {

class RequestTask : public engine::Task {
 public:
  using NotifyCb = std::function<void()>;

  RequestTask(engine::TaskProcessor* task_processor,
              const components::handlers::HandlerBase* handler,
              std::unique_ptr<request::RequestBase>&& request,
              NotifyCb&& notify_cb);
  virtual ~RequestTask() {}

  virtual void Run() noexcept override;
  virtual void Fail() noexcept override;
  virtual void OnComplete() noexcept override;

  void SetComplete();

  request::RequestBase& GetRequest() const { return *request_; }
  const components::handlers::HandlerBase* GetHandler() const {
    return handler_;
  }

 private:
  const components::handlers::HandlerBase* handler_ = nullptr;
  std::unique_ptr<request::RequestBase> request_;
  NotifyCb notify_cb_;
};

}  // namespace request_handling
}  // namespace server
