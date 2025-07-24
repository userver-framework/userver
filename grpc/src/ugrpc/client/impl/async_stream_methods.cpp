#include <userver/ugrpc/client/impl/async_stream_methods.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::impl {

void CheckOk(StreamingCallState& state, ugrpc::impl::AsyncMethodInvocation::WaitStatus status, std::string_view stage) {
    if (status == ugrpc::impl::AsyncMethodInvocation::WaitStatus::kError) {
        state.SetFinished();
        ProcessNetworkError(state, stage);
        throw RpcInterruptedError(state.GetCallName(), stage);
    } else if (status == ugrpc::impl::AsyncMethodInvocation::WaitStatus::kCancelled) {
        state.SetFinished();
        ProcessCancelled(state, stage);
        throw RpcCancelledError(state.GetCallName(), stage);
    }
}

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
