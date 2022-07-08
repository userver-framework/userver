#include <userver/clients/http/response_future.hpp>

#include <clients/http/easy_wrapper.hpp>
#include <clients/http/request_state.hpp>

USERVER_NAMESPACE_BEGIN

namespace clients::http {

ResponseFuture::ResponseFuture(
    engine::Future<std::shared_ptr<Response>>&& future,
    std::chrono::milliseconds total_timeout,
    std::shared_ptr<RequestState> request_state)
    : future_(std::move(future)),
      deadline_(engine::Deadline::FromDuration(total_timeout)),
      request_state_(std::move(request_state)) {}

ResponseFuture::ResponseFuture(ResponseFuture&& other) noexcept {
  std::swap(future_, other.future_);
  std::swap(deadline_, other.deadline_);
  std::swap(request_state_, other.request_state_);
}

ResponseFuture& ResponseFuture::operator=(ResponseFuture&& other) noexcept {
  if (&other == this) return *this;
  Cancel();
  future_ = std::move(other.future_);
  deadline_ = other.deadline_;
  request_state_ = std::move(other.request_state_);
  return *this;
}

ResponseFuture::~ResponseFuture() { Cancel(); }

void ResponseFuture::Cancel() {
  if (request_state_) {
    request_state_->Cancel();
  }
  Detach();
}

void ResponseFuture::Detach() {
  future_ = {};
  request_state_.reset();
}

std::future_status ResponseFuture::Wait() {
  auto status = future_.wait_until(deadline_);
  if (status == engine::FutureStatus::kCancelled) {
    const auto stats = request_state_->easy().get_local_stats();

    // request_ has armed timers to retry the request. Stopping those ASAP.
    Cancel();

    throw CancelException(
        "HTTP response wait was aborted due to task cancellation", stats);
  }
  return (status == engine::FutureStatus::kReady) ? std::future_status::ready
                                                  : std::future_status::timeout;
}

std::shared_ptr<Response> ResponseFuture::Get() {
  const auto future_status = Wait();
  if (future_status == std::future_status::ready) {
    auto response = future_.get();
    Detach();
    return response;
  }

  throw TimeoutException("Future timeout", {});  // no local stats available
}

engine::impl::ContextAccessor*
ResponseFuture::TryGetContextAccessor() noexcept {
  return future_.TryGetContextAccessor();
}

}  // namespace clients::http

USERVER_NAMESPACE_END
