#include <clients/grpc/detail/async_invocation.hpp>

#include <engine/blocking_future.hpp>

namespace clients::grpc::detail {

struct FutureWrapperAny::Impl {
  engine::impl::BlockingFuture<boost::any> future;
};

struct PromiseWrapperAny::Impl {
  engine::impl::BlockingPromise<boost::any> promise;
};

// template <>
struct FutureWrapper<void>::Impl {
  engine::impl::BlockingFuture<void> future;
};

// template <>
struct PromiseWrapper<void>::Impl {
  engine::impl::BlockingPromise<void> promise;
};

// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
FutureWrapperAny::FutureWrapperAny() = default;
// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
FutureWrapperAny::FutureWrapperAny(FutureWrapperAny&&) noexcept = default;
FutureWrapperAny::FutureWrapperAny(Impl&& impl) : pimpl_{std::move(impl)} {}
FutureWrapperAny::~FutureWrapperAny() = default;

FutureWrapperAny& FutureWrapperAny::operator=(FutureWrapperAny&&) noexcept =
    default;

boost::any FutureWrapperAny::Get() { return pimpl_->future.get(); }
void FutureWrapperAny::Wait() { pimpl_->future.wait(); }

// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
FutureWrapper<void>::FutureWrapper() = default;
// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
FutureWrapper<void>::FutureWrapper(FutureWrapper&&) noexcept = default;
FutureWrapper<void>::FutureWrapper(Impl&& impl) : pimpl_{std::move(impl)} {}
FutureWrapper<void>::~FutureWrapper() = default;

FutureWrapper<void>& FutureWrapper<void>::operator=(FutureWrapper&&) noexcept =
    default;

void FutureWrapper<void>::Get() { pimpl_->future.get(); }
void FutureWrapper<void>::Wait() { pimpl_->future.wait(); }

// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
PromiseWrapperAny::PromiseWrapperAny() = default;
PromiseWrapperAny::~PromiseWrapperAny() = default;

void PromiseWrapperAny::SetValue(boost::any&& value) {
  pimpl_->promise.set_value(std::move(value));
}

void PromiseWrapperAny::SetException(std::exception_ptr ex) {
  pimpl_->promise.set_exception(std::move(ex));
}

FutureWrapperAny PromiseWrapperAny::GetFuture() {
  return FutureWrapperAny(FutureWrapperAny::Impl{pimpl_->promise.get_future()});
}

// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
PromiseWrapper<void>::PromiseWrapper() = default;
PromiseWrapper<void>::~PromiseWrapper() = default;

void PromiseWrapper<void>::SetValue() { pimpl_->promise.set_value(); }

void PromiseWrapper<void>::SetException(std::exception_ptr ex) {
  pimpl_->promise.set_exception(std::move(ex));
}

FutureWrapper<void> PromiseWrapper<void>::GetFuture() {
  return FutureWrapper<void>(
      FutureWrapper<void>::Impl{pimpl_->promise.get_future()});
}

}  // namespace clients::grpc::detail
