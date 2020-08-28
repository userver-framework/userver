#include <clients/http/response_future.hpp>

#include <engine/blocking_future.hpp>

namespace clients::http {

ResponseFuture::ResponseFuture(
    engine::impl::BlockingFuture<std::shared_ptr<Response>>&& future,
    std::chrono::milliseconds total_timeout, std::shared_ptr<EasyWrapper> easy)
    : future_(std::move(future)),
      deadline_(std::chrono::system_clock::now() + total_timeout),
      easy_(std::move(easy)) {}

// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init,hicpp-member-init)
ResponseFuture::ResponseFuture(ResponseFuture&& other) noexcept {
  std::swap(future_, other.future_);
  std::swap(deadline_, other.deadline_);
  std::swap(easy_, other.easy_);
}

ResponseFuture::~ResponseFuture() { Cancel(); }

ResponseFuture& ResponseFuture::operator=(ResponseFuture&& other) noexcept =
    default;

void ResponseFuture::Cancel() {
  if (easy_) easy_->Easy().cancel();
  Detach();
}

void ResponseFuture::Detach() {
  *future_ = {};
  easy_.reset();
}

std::future_status ResponseFuture::Wait() const {
  auto status = future_->wait_until(deadline_);
  if (status != std::future_status::ready &&
      engine::current_task::IsCancelRequested()) {
    throw CancelException(
        "HTTP response wait was aborted due to task cancellation");
  }
  return status;
}

std::shared_ptr<Response> ResponseFuture::Get() {
  const auto future_status = Wait();
  if (future_status == std::future_status::ready) {
    auto response = future_->get();
    easy_.reset();
    return response;
  }

  throw TimeoutException("Future timeout");
}

}  // namespace clients::http
