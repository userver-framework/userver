#include <userver/ugrpc/client/impl/async_method_invocation.hpp>

#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::impl {

void FinishAsyncMethodInvocation::Notify(bool ok) noexcept {
    finish_time_ = std::chrono::steady_clock::now();
    AsyncMethodInvocation::Notify(ok);
}

std::chrono::steady_clock::time_point FinishAsyncMethodInvocation::GetFinishTime() const noexcept {
    UASSERT_MSG(finish_time_.has_value(), "GetFinishTime should be called after invocation was notified");
    return *finish_time_;
}

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
