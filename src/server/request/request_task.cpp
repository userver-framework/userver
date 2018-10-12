#include "request_task.hpp"

#include <engine/async.hpp>
#include <server/request/response_base.hpp>

namespace server {
namespace request {

RequestTask::RequestTask(engine::TaskProcessor* task_processor,
                         const handlers::HandlerBase* handler,
                         std::unique_ptr<RequestBase>&& request,
                         NotifyCb&& notify_cb)
    : task_processor_(task_processor),
      handler_(handler),
      request_(std::move(request)),
      notify_cb_(std::move(notify_cb)),
      is_complete_(false) {}

void RequestTask::WaitForTaskStop() {
  if (async_task_.IsValid()) async_task_.Wait();
}

void RequestTask::Start(bool can_fail) {
  if (async_task_.IsFinished()) return;

  assert(!is_complete_);
  assert(task_processor_);
  assert(handler_);

  auto payload = [this] {
    request_->SetTaskStartTime();
    request::RequestContext context;
    handler_->HandleRequest(*request_, context);
    request_->SetResponseNotifyTime();
    request_->GetResponse().SetReady();
    if (notify_cb_) notify_cb_();
    handler_->OnRequestComplete(*request_, context);
    request_->SetCompleteNotifyTime();
    // XXX: this triggers task destruction, before the task is actually finished
    // there must be a more straightforward way
    SetComplete();
  };

  if (can_fail) {
    async_task_ = engine::Async(*task_processor_, std::move(payload));
  } else {
    async_task_ = engine::CriticalAsync(*task_processor_, std::move(payload));
  }
  if (async_task_.GetCancellationReason() ==
      engine::Task::CancellationReason::kOverload) {
    request_->GetResponse().SetStatusServiceUnavailable();
    request_->GetResponse().SetReady();
    request_->SetResponseNotifyTime();
    SetComplete();
  }
}

void RequestTask::SetComplete() {
  auto notify_cb = std::move(notify_cb_);
  is_complete_ = true;
  if (notify_cb) notify_cb();
}

}  // namespace request
}  // namespace server
