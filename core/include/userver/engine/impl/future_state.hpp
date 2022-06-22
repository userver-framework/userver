#pragma once

#include <atomic>
#include <exception>

#include <userver/engine/deadline.hpp>
#include <userver/engine/future_status.hpp>
#include <userver/engine/impl/context_accessor.hpp>
#include <userver/engine/impl/wait_list_fwd.hpp>
#include <userver/utils/result_store.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::impl {

class FutureStateBase : private engine::impl::ContextAccessor {
 public:
  bool IsReady() const noexcept final;

  [[nodiscard]] FutureStatus WaitUntil(Deadline deadline);

  void OnFutureCreated();

  // Internal helper for WaitAny/WaitAll
  ContextAccessor* TryGetContextAccessor() noexcept { return this; }

 protected:
  class WaitStrategy;

  FutureStateBase() noexcept;
  ~FutureStateBase();

  void LockResultStore();
  void ReleaseResultStore();
  void WaitForResult();

 private:
  void AppendWaiter(impl::TaskContext& context) noexcept final;
  void RemoveWaiter(impl::TaskContext& context) noexcept final;

  FastPimplWaitListLight finish_waiters_;
  std::atomic<bool> is_ready_;
  std::atomic<bool> is_result_store_locked_;
  std::atomic<bool> is_future_created_;
};

template <typename T>
class FutureState final : public FutureStateBase {
 public:
  [[nodiscard]] T Get();

  void SetValue(const T& value);
  void SetValue(T&& value);
  void SetException(std::exception_ptr&& ex);

 private:
  void RethrowErrorResult() const override;

  utils::ResultStore<T> result_store_;
};

template <>
class FutureState<void> final : public FutureStateBase {
 public:
  void Get();

  void SetValue();
  void SetException(std::exception_ptr&& ex);

 private:
  void RethrowErrorResult() const override;

  utils::ResultStore<void> result_store_;
};

template <typename T>
T FutureState<T>::Get() {
  WaitForResult();
  return result_store_.Retrieve();
}

template <typename T>
void FutureState<T>::SetValue(const T& value) {
  LockResultStore();
  result_store_.SetValue(value);
  ReleaseResultStore();
}

template <typename T>
void FutureState<T>::SetValue(T&& value) {
  LockResultStore();
  result_store_.SetValue(std::move(value));
  ReleaseResultStore();
}

template <typename T>
void FutureState<T>::SetException(std::exception_ptr&& ex) {
  LockResultStore();
  result_store_.SetException(std::move(ex));
  ReleaseResultStore();
}

template <typename T>
void FutureState<T>::RethrowErrorResult() const {
  (void)result_store_.Get();
}

inline void FutureState<void>::Get() {
  WaitForResult();
  return result_store_.Retrieve();
}

inline void FutureState<void>::SetValue() {
  LockResultStore();
  result_store_.SetValue();
  ReleaseResultStore();
}

inline void FutureState<void>::SetException(std::exception_ptr&& ex) {
  LockResultStore();
  result_store_.SetException(std::move(ex));
  ReleaseResultStore();
}

inline void FutureState<void>::RethrowErrorResult() const {
  (void)result_store_.Get();
}

}  // namespace engine::impl

USERVER_NAMESPACE_END
