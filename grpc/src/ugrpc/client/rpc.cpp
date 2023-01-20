#include <userver/ugrpc/client/rpc.hpp>

#include <userver/ugrpc/client/exceptions.hpp>
#include <userver/utils/fast_scope_guard.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client {

UnaryFuture::UnaryFuture(impl::RpcData& data) noexcept : data_(&data) {}

UnaryFuture::~UnaryFuture() noexcept {
  if (data_) {
    impl::RpcData::AsyncMethodInvocationGuard guard(*data_);
    impl::ProcessFinishResult(*data_, data_->GetAsyncMethodInvocation().Wait(),
                              data_->GetStatus(), false);
  }
}

UnaryFuture::UnaryFuture(UnaryFuture&& other) noexcept
    : data_(std::exchange(other.data_, nullptr)) {}

UnaryFuture& UnaryFuture::operator=(UnaryFuture&& other) noexcept {
  if (this == &other) return *this;
  [[maybe_unused]] auto for_destruction = std::move(*this);
  data_ = std::exchange(other.data_, nullptr);
  return *this;
}

void UnaryFuture::Get() {
  UINVARIANT(data_, "'Get' must be called only once");
  impl::RpcData::AsyncMethodInvocationGuard guard(*data_);
  auto& data = *std::exchange(data_, nullptr);
  auto& status = data.GetStatus();
  impl::ProcessFinishResult(data, data.GetAsyncMethodInvocation().Wait(),
                            status, true);
}

}  // namespace ugrpc::client

USERVER_NAMESPACE_END
