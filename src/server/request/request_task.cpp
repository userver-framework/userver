#include "request_task.hpp"

#include <server/request/response_base.hpp>

namespace server {
namespace request {

RequestTask::RequestTask(engine::TaskProcessor* task_processor,
                         const handlers::HandlerBase* handler,
                         std::unique_ptr<RequestBase>&& request,
                         NotifyCb&& notify_cb)
    : engine::Task(task_processor),
      handler_(handler),
      request_(std::move(request)),
      notify_cb_(std::move(notify_cb)) {}

void RequestTask::Run() noexcept {
  assert(handler_ != nullptr);
  request_->SetTaskStartTime();
  request::RequestContext context;
  handler_->HandleRequest(*request_, context);
  request_->SetResponseNotifyTime();
  request_->GetResponse().SetReady();
  if (notify_cb_) notify_cb_();
  handler_->OnRequestComplete(*request_, context);
}

void RequestTask::Fail() noexcept {
  // can't process request task due to high load
  request_->GetResponse().SetStatusServiceUnavailable();
  request_->GetResponse().SetReady();
  request_->SetResponseNotifyTime();
  SetComplete();
}

void RequestTask::OnComplete() noexcept {
  request_->SetCompleteNotifyTime();
  SetComplete();
}

void RequestTask::SetComplete() {
  auto notify_cb = std::move(notify_cb_);
  SetState(State::kComplete);
  if (notify_cb) notify_cb();
}

}  // namespace request
}  // namespace server
