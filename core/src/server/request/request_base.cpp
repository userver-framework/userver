#include <userver/server/request/request_base.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::request {

RequestBase::RequestBase() : start_time_(std::chrono::steady_clock::now()) {}

RequestBase::~RequestBase() = default;

void RequestBase::SetTaskCreateTime() {
  task_create_time_ = std::chrono::steady_clock::now();
}

void RequestBase::SetTaskStartTime() {
  task_start_time_ = std::chrono::steady_clock::now();
}

void RequestBase::SetResponseNotifyTime() {
  response_notify_time_ = std::chrono::steady_clock::now();
}

void RequestBase::SetStartSendResponseTime() {
  start_send_response_time_ = std::chrono::steady_clock::now();
}

void RequestBase::SetFinishSendResponseTime() {
  finish_send_response_time_ = std::chrono::steady_clock::now();
  AccountResponseTime();
}

}  // namespace server::request

USERVER_NAMESPACE_END
