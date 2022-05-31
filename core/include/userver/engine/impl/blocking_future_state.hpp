#pragma once

#include <atomic>
#include <exception>
#include <future>
#include <mutex>

#include <userver/engine/deadline.hpp>
#include <userver/engine/impl/condition_variable_any.hpp>
#include <userver/engine/impl/context_accessor.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/result_store.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::impl {

class BlockingFutureStateBase : private engine::impl::ContextAccessor {
 public:
  BlockingFutureStateBase() noexcept;

  bool IsReady() const noexcept final;

  void Wait();

  [[nodiscard]] std::future_status WaitUntil(Deadline deadline);

  void EnsureNotRetrieved();

  // Internal helper for WaitAny/WaitAll
  ContextAccessor* TryGetContextAccessor() noexcept { return this; }

 protected:
  class Lock final {
   public:
    explicit Lock(BlockingFutureStateBase& self);

    Lock(Lock&&) = delete;
    Lock& operator=(Lock&&) = delete;

    ~Lock();

   private:
    BlockingFutureStateBase& self_;
  };

  ~BlockingFutureStateBase();

 private:
  void AppendWaiter(impl::TaskContext& context) noexcept override;
  void RemoveWaiter(impl::TaskContext& context) noexcept override;
  void WakeupAllWaiters() override;
  bool IsWaitingEnabledFrom(const impl::TaskContext& context) const
      noexcept override;

  std::mutex mutex_;
  ConditionVariableAny<std::mutex> result_cv_;
  std::atomic<bool> is_ready_;
  std::atomic<bool> is_retrieved_;
  mutable FastPimplWaitListLight finish_waiters_;
};

template <typename T>
class BlockingFutureState final : public BlockingFutureStateBase {
 public:
  [[nodiscard]] T Get();

  void SetValue(const T& value);
  void SetValue(T&& value);
  void SetException(std::exception_ptr&& ex);

 private:
  utils::ResultStore<T> result_store_;
};

template <>
class BlockingFutureState<void> final : public BlockingFutureStateBase {
 public:
  void Get();

  void SetValue();
  void SetException(std::exception_ptr&& ex);

 private:
  utils::ResultStore<void> result_store_;
};

template <typename T>
T BlockingFutureState<T>::Get() {
  Wait();
  return result_store_.Retrieve();
}

template <typename T>
void BlockingFutureState<T>::SetValue(const T& value) {
  const Lock lock{*this};
  result_store_.SetValue(value);
}

template <typename T>
void BlockingFutureState<T>::SetValue(T&& value) {
  const Lock lock{*this};
  result_store_.SetValue(std::move(value));
}

template <typename T>
void BlockingFutureState<T>::SetException(std::exception_ptr&& ex) {
  const Lock lock{*this};
  result_store_.SetException(std::move(ex));
}

inline void BlockingFutureState<void>::Get() {
  Wait();
  result_store_.Retrieve();
}

inline void BlockingFutureState<void>::SetValue() {
  const Lock lock{*this};
  result_store_.SetValue();
}

inline void BlockingFutureState<void>::SetException(std::exception_ptr&& ex) {
  const Lock lock{*this};
  result_store_.SetException(std::move(ex));
}

}  // namespace engine::impl

USERVER_NAMESPACE_END
