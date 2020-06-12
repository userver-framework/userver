#include <server/request/response_base.hpp>

#include <utils/assert.hpp>

namespace server {
namespace request {

ResponseBase::ResponseBase(ResponseDataAccounter& data_accounter)
    : data_accounter_(data_accounter) {}

ResponseBase::~ResponseBase() noexcept { data_accounter_.Put(data_.size()); }

void ResponseBase::SetData(std::string data) {
  if (!data_.empty()) {
    data_accounter_.Put(data_.size());
  }
  data_ = std::move(data);
  data_accounter_.Get(data_.size());
}

void ResponseBase::SetReady() {
  ready_time_ = std::chrono::steady_clock::now();
  is_ready_ = true;
}

bool ResponseBase::IsLimitReached() const {
  return data_accounter_.GetCurrentLevel() >= data_accounter_.GetMaxLevel();
}

void ResponseBase::SetSendFailed(
    std::chrono::steady_clock::time_point failure_time) {
  SetSentTime(failure_time);
  SetSent(0);
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
