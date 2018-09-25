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

ResponseFuture::ResponseFuture(ResponseFuture&&) = default;

ResponseFuture::~ResponseFuture() {
  if (future_->IsValid()) {
    // TODO: log it?
    easy_->Easy().cancel();
  }
}

ResponseFuture& ResponseFuture::operator=(ResponseFuture&& other) = default;

void ResponseFuture::Cancel() {
  easy_->Easy().cancel();
  Detach();
}

void ResponseFuture::Detach() { *future_ = {}; }

std::future_status ResponseFuture::Wait() const {
  return future_->wait_until(deadline_);
}

std::shared_ptr<Response> ResponseFuture::Get() {
  if (Wait() == std::future_status::ready) return future_->get();
  throw TimeoutException("Future timeout");
}

}  // namespace http
}  // namespace clients
