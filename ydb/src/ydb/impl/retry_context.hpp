#pragma once

#include <ydb-cpp-sdk/client/retry/retry.h>

#include <userver/utils/retry_budget.hpp>
#include <userver/ydb/table.hpp>

#include <ydb/impl/request_context.hpp>

USERVER_NAMESPACE_BEGIN

namespace ydb::impl {

// See TRetryContextBase for an understanding of error handling
bool IsRetryableStatus(NYdb::EStatus status);

NYdb::NRetry::TRetryOperationSettings PrepareRetrySettings(const OperationSettings& settings, bool is_retryable);

inline NYdb::TStatus MakeNonRetryableStatus() { return NYdb::TStatus{NYdb::EStatus::BAD_REQUEST, NYql::TIssues()}; }

// Func: (NYdb::NTable::TSession) -> NThreading::TFuture<T>
//       OR
//       (NYdb::NTable::TTableClient&) -> NThreading::TFuture<T>
// RetryOperationWithResult -> NThreading::TFuture<T>
template <typename Func>
auto RetryOperation(impl::RequestContext& request_context, Func&& func) {
    using FuncArg = std::conditional_t<
        std::is_invocable_v<Func&, NYdb::NTable::TSession>,
        NYdb::NTable::TSession,
        NYdb::NTable::TTableClient&>;
    using FuncResultType = std::invoke_result_t<Func&, FuncArg>;
    using ResultType = typename FuncResultType::value_type;
    using AsyncResultType = NThreading::TFuture<ResultType>;

    static_assert(std::is_convertible_v<const ResultType&, NYdb::TStatus>);

    auto& retry_budget = request_context.table_client.GetRetryBudget();
    NYdb::NRetry::TRetryOperationSettings retry_settings{
        PrepareRetrySettings(request_context.settings, retry_budget.CanRetry())};

    auto final_result = utils::MakeSharedRef<std::optional<ResultType>>();

    return request_context.table_client.GetNativeTableClient()
        .RetryOperation(
            [final_result, func = std::forward<Func>(func), &retry_budget = retry_budget](FuncArg arg) mutable {
                // If `func` throws, `RetryOperation` will immediately rethrow
                // the exception without further retries.
                AsyncResultType result_future = func(std::forward<FuncArg>(arg));

                return result_future.Apply([final_result,
                                            &retry_budget = retry_budget](const AsyncResultType& const_fut) mutable {
                    // If `const_fut` contains an exception, `ExtractValue` call
                    // will rethrow it, and `Apply` will pack it into the
                    // resulting future.
                    //
                    // Alternatively, we could just copy the TFuture,
                    // inducing some pointless refcounting overhead.
                    auto value =
                        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
                        const_cast<AsyncResultType&>(const_fut).ExtractValue();
                    // NOLINTNEXTLINE(cppcoreguidelines-slicing)
                    NYdb::TStatus status{value};
                    final_result->emplace(std::move(value));
                    if (status.IsSuccess()) {
                        retry_budget.AccountOk();
                    } else if (IsRetryableStatus(status.GetStatus())) {
                        if (retry_budget.CanRetry()) {
                            retry_budget.AccountFail();
                        } else {
                            return MakeNonRetryableStatus();
                        }
                    }
                    return status;
                });
            },
            retry_settings
        )
        .Apply([final_result = std::move(final_result)](const NYdb::TAsyncStatus& const_fut) {
            const_fut.TryRethrow();
            if (final_result->has_value()) {
                return std::move(**final_result);
            }
            throw DeadlineExceededError("Timed out before the initial attempt in RetryOperation");
        });
}

}  // namespace ydb::impl

USERVER_NAMESPACE_END
