#include <userver/clients/grpc/impl/async_methods.hpp>

#include <userver/utils/assert.hpp>

namespace clients::grpc::impl {

void AsyncMethodInvocation::Notify(bool ok) noexcept {
  ok_ = ok;
  event_.Send();
}

void* AsyncMethodInvocation::GetTag() noexcept {
  return static_cast<void*>(this);
}

bool AsyncMethodInvocation::Wait() noexcept {
  event_.WaitNonCancellable();
  return ok_;
}

void ProcessStartCallResult(std::string_view call_name, bool ok) {
  if (!ok) {
    throw UnknownRpcError(call_name, "StartCall");
  }
}

void ProcessFinishResult(std::string_view call_name, bool ok,
                         ::grpc::Status&& status) {
  if (!ok) {
    throw UnknownRpcError(call_name, "Finish");
  } else if (!status.ok()) {
    impl::ThrowErrorWithStatus(call_name, std::move(status));
  }
}

}  // namespace clients::grpc::impl
