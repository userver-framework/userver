#include <userver/ugrpc/client/rpc.hpp>

#include <userver/utils/fast_scope_guard.hpp>

#include <ugrpc/impl/internal_tag.hpp>
#include <userver/ugrpc/client/exceptions.hpp>
#include <userver/ugrpc/client/middlewares/base.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client {

UnaryFuture::UnaryFuture(impl::RpcData& data) noexcept : impl_(data) {}

UnaryFuture::~UnaryFuture() noexcept {
  if (auto* const data = impl_.GetData()) {
    impl::RpcData::AsyncMethodInvocationGuard guard(*data);

    auto& finish = data->GetFinishAsyncMethodInvocation();
    auto& status = finish.GetStatus();

    impl::ProcessFinishResult(*data, impl::Wait(finish, data->GetContext()),
                              std::move(status),
                              std::move(finish.GetParsedGStatus()), false);
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

  auto& finish = data->GetFinishAsyncMethodInvocation();

  const auto wait_status = impl::Wait(finish, data->GetContext());
  if (wait_status == impl::AsyncMethodInvocation::WaitStatus::kCancelled) {
    data->GetStatsScope().OnCancelled();
    throw RpcCancelledError(data->GetCallName(), "Get()");
  }

  impl::ProcessFinishResult(*data, wait_status, std::move(finish.GetStatus()),
                            std::move(finish.GetParsedGStatus()), true);
}

bool UnaryFuture::IsReady() const noexcept { return impl_.IsReady(); }

namespace impl {

void CallMiddlewares(const Middlewares& mws, CallAnyBase& call,
                     utils::function_ref<void()> user_call,
                     const ::google::protobuf::Message* request) {
  MiddlewareCallContext mw_ctx(mws, call, user_call, request);
  mw_ctx.Next();
}

}  // namespace impl

grpc::ClientContext& CallAnyBase::GetContext() { return data_->GetContext(); }

impl::RpcData& CallAnyBase::GetData() {
  UASSERT(data_);
  return *data_;
}

impl::RpcData& CallAnyBase::GetData(ugrpc::impl::InternalTag) {
  UASSERT(data_);
  return *data_;
}

std::string_view CallAnyBase::GetCallName() const {
  UASSERT(data_);
  return data_->GetCallName();
}

std::string_view CallAnyBase::GetClientName() const {
  UASSERT(data_);
  return data_->GetClientName();
}

}  // namespace ugrpc::client

USERVER_NAMESPACE_END
