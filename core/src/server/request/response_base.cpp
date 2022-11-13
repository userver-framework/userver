#include <userver/server/request/response_base.hpp>

#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::request {

namespace {
const auto kStartTime = std::chrono::steady_clock::now();

std::chrono::milliseconds ToMsFromStart(
    std::chrono::steady_clock::time_point tp) {
  return std::chrono::duration_cast<std::chrono::milliseconds>(tp - kStartTime);
}

}  // namespace

void ResponseDataAccounter::StartRequest(
    size_t size, std::chrono::steady_clock::time_point create_time) {
  count_++;
  current_ += size;
  auto ms = ToMsFromStart(create_time);
  time_sum_ += ms.count();
}

void ResponseDataAccounter::StopRequest(
    size_t size, std::chrono::steady_clock::time_point create_time) {
  current_ -= size;
  auto ms = ToMsFromStart(create_time);
  time_sum_ -= ms.count();
  count_--;
}

std::chrono::milliseconds ResponseDataAccounter::GetAvgRequestTime() const {
  // TODO: race
  auto count = count_.load();
  auto time_sum = std::chrono::milliseconds(time_sum_.load());

  auto now_ms = ToMsFromStart(std::chrono::steady_clock::now());
  auto delta = (now_ms * count) - time_sum;
  return delta / (count ? count : 1);
}

ResponseBase::ResponseBase(ResponseDataAccounter& data_accounter)
    : accounter_(data_accounter),
      create_time_(std::chrono::steady_clock::now()) {
  guard_.emplace(accounter_, create_time_, data_.size());
}

ResponseBase::~ResponseBase() noexcept = default;

void ResponseBase::SetData(std::string data) {
  create_time_ = std::chrono::steady_clock::now();
  data_ = std::move(data);
  guard_.emplace(accounter_, create_time_, data_.size());
}

void ResponseBase::SetReady() { SetReady(std::chrono::steady_clock::now()); }

void ResponseBase::SetReady(std::chrono::steady_clock::time_point now) {
  ready_time_ = now;
  is_ready_ = true;
}

bool ResponseBase::IsLimitReached() const {
  return accounter_.GetCurrentLevel() >= accounter_.GetMaxLevel();
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

}  // namespace server::request

USERVER_NAMESPACE_END
