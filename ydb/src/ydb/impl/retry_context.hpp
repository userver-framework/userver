#pragma once

#include <ydb-cpp-sdk/client/retry/retry.h>

#include <userver/ydb/table.hpp>

#include <ydb/impl/request_context.hpp>

USERVER_NAMESPACE_BEGIN

namespace ydb::impl {

// See TRetryContextBase for an understanding of error handling
bool IsRetryableStatus(NYdb::EStatus status);

void HandleOnceRetry(utils::RetryBudget& retry_budget, NYdb::EStatus status);

struct RetryContext final {
  RetryContext(TableClient& table_client, OperationSettings& settings);

  bool ApplyStatusAndCanRetry(NYdb::EStatus status);

  utils::RetryBudget& retry_budget;
  NYdb::NRetry::TRetryOperationSettings retry_settings;
  std::size_t retries{0};
};

inline NThreading::TFuture<NYdb::TStatus> MakeNonRetryableFuture() {
  return NThreading::MakeFuture<NYdb::TStatus>(
      NYdb::TStatus{NYdb::EStatus::BAD_REQUEST, NYql::TIssues()});
}

// Func: (NYdb::NTable::TSession) -> NThreading::TFuture<T>
//       OR
//       (NYdb::NTable::TTableClient&) -> NThreading::TFuture<T>
// RetryOperationWithResult -> NThreading::TFuture<T>
template <typename Func>
auto RetryOperation(impl::RequestContext& request_context, Func&& func) {
  using FuncArg =
      std::conditional_t<std::is_invocable_v<Func&, NYdb::NTable::TSession>,
                         NYdb::NTable::TSession, NYdb::NTable::TTableClient&>;
  using FuncResultType = std::invoke_result_t<Func&, FuncArg>;
  using ResultType = typename FuncResultType::value_type;
  using AsyncResultType = NThreading::TFuture<ResultType>;

  static_assert(std::is_convertible_v<const ResultType&, NYdb::TStatus>);

  RetryContext retry_context{request_context.table_client,
                             request_context.settings};

  if constexpr (std::is_same_v<ResultType, NYdb::TStatus>) {
    return request_context.table_client.GetNativeTableClient().RetryOperation(
        [func = std::forward<Func>(func),
         retry_context = std::move(retry_context)](FuncArg arg) mutable {
          return func(std::forward<FuncArg>(arg))
              // We inject this handling to manage retries
              .Apply([retry_context = &retry_context](const auto& f) mutable {
                if (!retry_context->ApplyStatusAndCanRetry(
                        f.GetValue().GetStatus())) {
                  // Stop retrying
                  return MakeNonRetryableFuture();
                }
                return f;
              });
        },
        retry_context.retry_settings);
  }

  auto final_result = utils::MakeSharedRef<std::optional<ResultType>>();

  return request_context.table_client.GetNativeTableClient()
      .RetryOperation(
          [final_result, func = std::forward<Func>(func),
           retry_context = std::move(retry_context)](FuncArg arg) mutable {
            // If `func` throws, `RetryOperation` will immediately rethrow
            // the exception without further retries.
            AsyncResultType result_future = func(std::forward<FuncArg>(arg));

            return result_future.Apply(
                [final_result, retry_context = &retry_context](
                    const AsyncResultType& const_fut) mutable {
                  // Alternatively, we could just copy the TFuture,
                  // inducing some pointless refcounting overhead.
                  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
                  auto& fut = const_cast<AsyncResultType&>(const_fut);
                  // If `fut` contains an exception, `ExtractValue` call
                  // will rethrow it, and `Apply` will pack it into the
                  // resulting future.
                  auto value = fut.ExtractValue();
                  if (!retry_context->ApplyStatusAndCanRetry(
                          value.GetStatus())) {
                    final_result->emplace(std::move(value));
                    // Stop retrying
                    return MakeNonRetryableFuture();
                  }
                  // Note: slicing.
                  auto status_future =
                      NThreading::MakeFuture<NYdb::TStatus>(value);
                  final_result->emplace(std::move(value));
                  return status_future;
                });
          },
          retry_context.retry_settings)
      .Apply([final_result = std::move(final_result)](
                 const NYdb::TAsyncStatus& const_fut) {
        const_fut.TryRethrow();
        if (final_result->has_value()) {
          return std::move(**final_result);
        }
        throw DeadlineExceededError(
            "Timed out before the initial attempt in RetryOperation");
      });
}

}  // namespace ydb::impl

USERVER_NAMESPACE_END
