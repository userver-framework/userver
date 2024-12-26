#pragma once

#include <memory>

#include <ydb-cpp-sdk/client/retry/retry.h>
#include <ydb-cpp-sdk/client/table/table.h>

#include <userver/utils/retry_budget.hpp>

#include <ydb/impl/request_context.hpp>

USERVER_NAMESPACE_BEGIN

namespace ydb::impl {

NYdb::NRetry::TRetryOperationSettings
PrepareRetrySettings(const OperationSettings& settings, const utils::RetryBudget& retry_budget);

// See TRetryContextBase for an understanding of error handling
bool IsRetryableStatus(NYdb::EStatus status);

NYdb::TStatus MakeNonRetryableStatus();

template <typename... Args>
NYdb::TAsyncStatus RetryOperation(NYdb::NTable::TTableClient& table_client, Args&&... args) {
    return table_client.RetryOperation(std::forward<Args>(args)...);
}

template <typename... Args>
NYdb::TAsyncStatus RetryOperation(NYdb::NQuery::TQueryClient& query_client, Args&&... args) {
    return query_client.RetryQuery(std::forward<Args>(args)...);
}

template <typename Client, typename Fn>
class RetryHandler : public std::enable_shared_from_this<RetryHandler<Client, Fn>> {
public:
    using TSession = typename Client::TSession;
    using ArgType = std::conditional_t<std::is_invocable_v<Fn&, TSession>, TSession, Client&>;
    using AsyncResultType = std::invoke_result_t<Fn&, ArgType>;
    using ResultType = typename AsyncResultType::value_type;

    RetryHandler(
        Client& client,
        utils::RetryBudget& retry_budget,
        const NYdb::NRetry::TRetryOperationSettings& retry_settings,
        Fn&& fn
    )
        : client_{client}, retry_budget_{retry_budget}, retry_settings_{retry_settings}, fn_{std::move(fn)} {}

    AsyncResultType Execute() {
        auto internal_retry_status = RetryOperation(
            client_,
            [ctx = this->shared_from_this()](ArgType arg) {
                return ctx->InternalRetryIteration(std::forward<ArgType>(arg));
            },
            retry_settings_
        );

        return internal_retry_status.Apply([ctx = this->shared_from_this()](
                                               const NYdb::TAsyncStatus& internal_async_status
                                           ) { return ctx->TransformInternalRetryStatus(internal_async_status); });
    }

private:
    NYdb::TAsyncStatus InternalRetryIteration(ArgType arg) {
        const auto async_result = fn_(std::forward<ArgType>(arg));

        return async_result.Apply([ctx = this->shared_from_this()](const auto& async_result) {
            return ctx->HandleResult(async_result);
        });
    }

    NYdb::TStatus HandleResult(const AsyncResultType& async_result) {
        // If `async_result` contains an exception, `ExtractValue` call
        // will rethrow it, and `Apply` will pack it into the
        // resulting future.
        //
        // Alternatively, we could just copy the TFuture, causing pointless refcounting overhead.
        //
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
        result_.emplace(const_cast<AsyncResultType&>(async_result).ExtractValue());

        if (result_->IsSuccess()) {
            retry_budget_.AccountOk();
        } else if (IsRetryableStatus(result_->GetStatus())) {
            if (retry_budget_.CanRetry()) {
                retry_budget_.AccountFail();
            } else {
                return MakeNonRetryableStatus();
            }
        }

        // NOLINTNEXTLINE(cppcoreguidelines-slicing)
        return NYdb::TStatus{*result_};
    }

    ResultType TransformInternalRetryStatus(const NYdb::TAsyncStatus& internal_retry_status) {
        internal_retry_status.TryRethrow();

        if (!result_.has_value()) {
            throw DeadlineExceededError("Timed out before the initial attempt in RetryOperation");
        }

        return std::move(*result_);
    }

    Client& client_;
    utils::RetryBudget& retry_budget_;
    NYdb::NRetry::TRetryOperationSettings retry_settings_;
    Fn fn_;

    std::optional<ResultType> result_;
};

// Fn: (NYdb::NTable::TSession) -> NThreading::TFuture<T>
//     OR
//     (NYdb::NTable::TTableClient&) -> NThreading::TFuture<T>
// RetryOperation -> NThreading::TFuture<T>
template <typename Fn>
auto RetryOperation(impl::RequestContext& request_context, Fn&& fn) {
    static_assert(std::is_invocable_v<Fn&, NYdb::NTable::TSession> || std::is_invocable_v<Fn&, NYdb::NTable::TTableClient&>);

    auto& client = request_context.table_client.GetNativeTableClient();
    auto& retry_budget = request_context.table_client.GetRetryBudget();
    auto ctx = std::make_shared<RetryHandler<NYdb::NTable::TTableClient, Fn>>(
        client, retry_budget, PrepareRetrySettings(request_context.settings, retry_budget), std::forward<Fn>(fn)
    );
    return ctx->Execute();
}

template <typename Fn>
auto RetryQueryOperation(impl::RequestContext& request_context, Fn&& fn) {
    static_assert(std::is_invocable_v<Fn&, NYdb::NQuery::TSession> || std::is_invocable_v<Fn&, NYdb::NQuery::TQueryClient&>);

    auto& client = request_context.table_client.GetNativeQueryClient();
    auto& retry_budget = request_context.table_client.GetRetryBudget();
    auto ctx = std::make_shared<RetryHandler<NYdb::NQuery::TQueryClient, Fn>>(
        client, retry_budget, PrepareRetrySettings(request_context.settings, retry_budget), std::forward<Fn>(fn)
    );
    return ctx->Execute();
}

}  // namespace ydb::impl

USERVER_NAMESPACE_END
