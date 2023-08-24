#include <userver/clients/http/response_future.hpp>

#include <algorithm>

#include <clients/http/request_state.hpp>
#include <userver/server/request/task_inherited_data.hpp>
#include <userver/utils/fast_scope_guard.hpp>

USERVER_NAMESPACE_BEGIN

namespace clients::http {

namespace {

/// Max number of retries during calculating timeout
constexpr int kMaxRetryInTimeout = 5;
/// Base time for exponential backoff algorithm
constexpr long kEBBaseTime = 25;

long MaxRetryTime(short retries) {
  long time_ms = 0;
  for (short int i = 1; i < retries; ++i) {
    time_ms += kEBBaseTime * ((1 << std::min(i - 1, kMaxRetryInTimeout)) + 1);
  }
  return time_ms;
}

long CompleteTimeout(long request_timeout, short retries) {
  return static_cast<long>(static_cast<double>(request_timeout * retries) *
                           1.1) +
         MaxRetryTime(retries);
}

engine::Deadline ComputeBaseDeadline(RequestState& request_state) {
  return engine::Deadline::FromDuration(std::chrono::milliseconds(
      CompleteTimeout(request_state.timeout(), request_state.retries())));
}

}  // namespace

ResponseFuture::ResponseFuture(
    engine::Future<std::shared_ptr<Response>>&& future,
    std::shared_ptr<RequestState> request_state)
    : future_(std::move(future)),
      deadline_(ComputeBaseDeadline(*request_state)),
      request_state_(std::move(request_state)) {
  const auto propagated_deadline = request_state_->GetDeadline();
  if (propagated_deadline < deadline_) {
    deadline_ = propagated_deadline;
    was_deadline_propagated_ = true;
  }
}

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
  was_deadline_propagated_ = other.was_deadline_propagated_;
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
  switch (future_.wait_until(deadline_)) {
    case engine::FutureStatus::kCancelled: {
      const auto stats = request_state_->easy().get_local_stats();

      // request_ has armed timers to retry the request. Stopping those ASAP.
      Cancel();

      throw CancelException(
          "HTTP response wait was aborted due to task cancellation", stats);
    }
    case engine::FutureStatus::kTimeout:
      if (was_deadline_propagated_) {
        server::request::MarkTaskInheritedDeadlineExpired();

        // We allow the physical HTTP request to complete in the background to
        // avoid closing the connection.
        const utils::FastScopeGuard detach_guard(
            [this]() noexcept { Detach(); });

        request_state_->ThrowDeadlineExpiredException();
      }
      return std::future_status::timeout;
    case engine::FutureStatus::kReady:
      return std::future_status::ready;
  }

  UINVARIANT(false, "Invalid engine::FutureStatus");
}

std::shared_ptr<Response> ResponseFuture::Get() {
  const auto future_status = Wait();
  if (future_status == std::future_status::ready) {
    if (request_state_->IsDeadlineExpired()) {
      server::request::MarkTaskInheritedDeadlineExpired();
    }
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
