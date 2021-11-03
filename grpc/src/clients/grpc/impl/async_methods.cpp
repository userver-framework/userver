#include <userver/clients/grpc/impl/async_methods.hpp>

#include <userver/utils/assert.hpp>

#include <userver/clients/grpc/exceptions.hpp>

USERVER_NAMESPACE_BEGIN

namespace clients::grpc::impl {

void ProcessStartCallResult(std::string_view call_name, bool ok) {
  if (!ok) {
    throw RpcInterruptedError(call_name,
                              "StartCall (while starting the stream)");
  }
}

void ProcessFinishResult(std::string_view call_name, bool ok,
                         ::grpc::Status&& status) {
  UASSERT_MSG(ok,
              "ok=false in async Finish method invocation is prohibited "
              "by gRPC docs, see ::grpc::CompletionQueue::Next");
  if (!status.ok()) {
    impl::ThrowErrorWithStatus(call_name, std::move(status));
  }
}

}  // namespace clients::grpc::impl

USERVER_NAMESPACE_END
