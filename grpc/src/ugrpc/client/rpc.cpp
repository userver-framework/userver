#include <userver/ugrpc/client/rpc.hpp>

#include <userver/ugrpc/client/exceptions.hpp>
#include <userver/utils/fast_scope_guard.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client {

UnaryFuture::UnaryFuture(impl::RpcData& data) noexcept : impl_(data) {}

UnaryFuture::~UnaryFuture() noexcept {
  if (auto* const data = impl_.GetData()) {
    impl::RpcData::AsyncMethodInvocationGuard guard(*data);
    impl::ProcessFinishResult(*data, data->GetAsyncMethodInvocation().Wait(),
                              data->GetStatus(), false);
  }
}

UnaryFuture& UnaryFuture::operator=(UnaryFuture&& other) noexcept {
  if (this == &other) return *this;
  [[maybe_unused]] auto for_destruction = std::move(*this);
  impl_ = std::move(other.impl_);
  return *this;
}

void UnaryFuture::Get() {
  auto* const data = impl_.GetData();
  UINVARIANT(data, "'Get' must be called only once");
  impl::RpcData::AsyncMethodInvocationGuard guard(*data);
  impl_.ClearData();
  auto& status = data->GetStatus();
  impl::ProcessFinishResult(*data, data->GetAsyncMethodInvocation().Wait(),
                            status, true);
}

bool UnaryFuture::IsReady() const noexcept { return impl_.IsReady(); }

}  // namespace ugrpc::client

USERVER_NAMESPACE_END
