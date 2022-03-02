#include <userver/ugrpc/client/impl/async_methods.hpp>

#include <userver/utils/assert.hpp>

#include <userver/ugrpc/client/exceptions.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::impl {

void ProcessStartCallResult(std::string_view call_name,
                            ugrpc::impl::RpcStatisticsScope& stats, bool ok) {
  if (!ok) {
    stats.OnNetworkError();
    throw RpcInterruptedError(call_name,
                              "StartCall (while starting the stream)");
  }
}

void ProcessFinishResult(std::string_view call_name,
                         ugrpc::impl::RpcStatisticsScope& stats, bool ok,
                         grpc::Status&& status) {
  UASSERT_MSG(ok,
              "ok=false in async Finish method invocation is prohibited "
              "by gRPC docs, see grpc::CompletionQueue::Next");
  stats.OnExplicitFinish(status.error_code());
  if (!status.ok()) {
    impl::ThrowErrorWithStatus(call_name, std::move(status));
  }
}

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
