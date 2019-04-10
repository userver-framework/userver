#include <clients/http/response_future.hpp>

#include <engine/future.hpp>

namespace clients {
namespace http {

ResponseFuture::ResponseFuture(
    engine::Future<std::shared_ptr<Response>>&& future,
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

ResponseFuture::~ResponseFuture() {
  if (future_->IsValid()) {
    // TODO: log it?
    easy_->Easy().cancel();
  }
}

ResponseFuture& ResponseFuture::operator=(ResponseFuture&& other) noexcept =
    default;

void ResponseFuture::Cancel() {
  easy_->Easy().cancel();
  Detach();
}

void ResponseFuture::Detach() { *future_ = {}; }

std::future_status ResponseFuture::Wait() const {
  return future_->wait_until(deadline_);
}

std::shared_ptr<Response> ResponseFuture::Get() {
  const auto future_status = Wait();
  if (future_status == std::future_status::ready) {
    auto response = future_->get();

    return response;
  }

  throw TimeoutException("Future timeout");
}

}  // namespace http
}  // namespace clients
