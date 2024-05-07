#pragma once

#include <ydb-cpp-sdk/client/retry/retry.h>
#include <ydb-cpp-sdk/client/table/table.h>
#include <ydb-cpp-sdk/library/threading/future/core/future.h>

#include <userver/drivers/subscribable_futures.hpp>
#include <userver/utils/not_null.hpp>

#include <userver/ydb/exceptions.hpp>
#include <ydb/impl/retry_context.hpp>

USERVER_NAMESPACE_BEGIN

namespace ydb::impl {

template <typename T>
void WaitForFuture(const NThreading::TFuture<T>& future) {
  if (engine::FutureStatus::kReady !=
      drivers::TryWaitForSubscribableFuture(future, engine::Deadline{})) {
    throw OperationCancelledError();
  }
}

template <typename T>
T GetFutureValue(NThreading::TFuture<T>&& future) {
  static_assert(!std::is_base_of_v<NYdb::TStatus, T>,
                "for T derived from NYdb::TStatus use GetFutureValueUnchecked");
  impl::WaitForFuture(future);
  if constexpr (std::is_void_v<T>) {
    future.GetValue();
  } else {
    return future.ExtractValue();
  }
}

template <typename T>
T GetFutureValueUnchecked(NThreading::TFuture<T>&& future) {
  impl::WaitForFuture(future);
  return future.ExtractValue();
}

template <typename T>
T GetFutureValueChecked(NThreading::TFuture<T>&& future,
                        std::string_view operation_name) {
  auto status = impl::GetFutureValueUnchecked(std::move(future));
  if (!status.IsSuccess()) {
    throw YdbResponseError{operation_name, std::move(status)};
  }
  return status;
}

template <typename T>
T GetFutureValueChecked(NThreading::TFuture<T>&& future,
                        std::string_view operation_name,
                        utils::RetryBudget& retry_budget) {
  auto status = GetFutureValueUnchecked(std::move(future));
  HandleOnceRetry(retry_budget, status.GetStatus());
  if (!status.IsSuccess()) {
    throw YdbResponseError{operation_name, std::move(status)};
  }
  return status;
}

}  // namespace ydb::impl

USERVER_NAMESPACE_END
