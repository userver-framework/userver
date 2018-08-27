#include <yandex/taxi/userver/server/request/request_base.hpp>

namespace server {
namespace request {

RequestBase::RequestBase() : start_time_(std::chrono::steady_clock::now()) {}

RequestBase::~RequestBase() {}

void RequestBase::SetTaskCreateTime() {
  task_create_time_ = std::chrono::steady_clock::now();
}

void RequestBase::SetTaskStartTime() {
  task_start_time_ = std::chrono::steady_clock::now();
}

void RequestBase::SetResponseNotifyTime() {
  response_notify_time_ = std::chrono::steady_clock::now();
}

void RequestBase::SetCompleteNotifyTime() {
  complete_notify_time_ = std::chrono::steady_clock::now();
}

void RequestBase::SetStartSendResponseTime() {
  start_send_response_time_ = std::chrono::steady_clock::now();
}

void RequestBase::SetFinishSendResponseTime() {
  finish_send_response_time_ = std::chrono::steady_clock::now();
}

}  // namespace request
}  // namespace server
