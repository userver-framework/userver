#include "response_base.hpp"

namespace server {
namespace request {

void ResponseBase::SetData(std::string data) { data_ = std::move(data); }

void ResponseBase::SetReady() {
  ready_time_ = std::chrono::steady_clock::now();
  is_ready_ = true;
}

void ResponseBase::SetSent(size_t bytes_sent) {
  bytes_sent_ = bytes_sent;
  is_sent_ = true;
}

void ResponseBase::SetSentTime(
    std::chrono::steady_clock::time_point sent_time) {
  sent_time_ = sent_time;
}

}  // namespace request
}  // namespace server
