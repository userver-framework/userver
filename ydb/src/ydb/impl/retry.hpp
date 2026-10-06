#pragma once

#include <memory>
#include <optional>
#include <type_traits>
#include <utility>

#include <library/cpp/threading/future/core/future.h>
#include <util/datetime/base.h>
#include <ydb-cpp-sdk/client/query/client.h>
#include <ydb-cpp-sdk/client/retry/retry.h>
#include <ydb-cpp-sdk/client/table/table.h>

#include <userver/utils/retry_budget.hpp>
#include <userver/ydb/exceptions.hpp>

#include <ydb/impl/request_context.hpp>

USERVER_NAMESPACE_BEGIN

namespace ydb::impl {

NYdb::NRetry::TRetryOperationSettings PrepareRetrySettings(
    const OperationSettings& settings,
    const utils::RetryBudget& retry_budget,
    engine::Deadline deadline
);

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

template <typename TClient, typename Fn>
class RetryHandler : public std::enable_shared_from_this<RetryHandler<TClient, Fn>> {
public:
    using TSession = typename TClient::TSession;
    using ArgType = std::conditional_t<std::is_invocable_v<Fn&, TSession>, TSession, TClient&>;
    using AsyncResultType = std::invoke_result_t<Fn&, ArgType>;
    using ResultType = typename AsyncResultType::value_type;

    RetryHandler(
        TClient& client,
        utils::RetryBudget& retry_budget,
        const NYdb::NRetry::TRetryOperationSettings& retry_settings,
        Fn&& fn
    )
        : client_{client},
          retry_budget_{retry_budget},
          retry_settings_{retry_settings},
          fn_{std::move(fn)}
    {}

    AsyncResultType Execute() {
        return Execute([](TClient& client, auto operation, const auto& settings) {
            return RetryOperation(client, std::move(operation), settings);
        });
    }

    // The injected attempt lets tests simulate session acquisition failures before the operation callback.
    template <typename ExecuteAttempt>
    AsyncResultType Execute(ExecuteAttempt execute_attempt) {
        const auto deadline = TInstant::Now() + retry_settings_.MaxTimeout_;
        auto internal_retry_status = RetryOperation(
            client_,
            [handler = this->shared_from_this(),
             execute_attempt = std::move(execute_attempt),
             deadline](TClient&) mutable { return handler->InternalRetryIteration(execute_attempt, deadline); },
            retry_settings_
        );

        return internal_retry_status
            .Apply([handler = this->shared_from_this()](const NYdb::TAsyncStatus& internal_async_status) {
                return handler->TransformInternalRetryStatus(internal_async_status);
            });
    }

private:
    NYdb::NRetry::TRetryOperationSettings MakeSingleAttemptSettings(TInstant deadline) const {
        auto settings = retry_settings_;
        settings.MaxRetries(0);
        settings.MaxTimeout(deadline - TInstant::Now());
        return settings;
    }

    template <typename ExecuteAttempt>
    NYdb::TAsyncStatus InternalRetryIteration(ExecuteAttempt& execute_attempt, TInstant deadline) {
        if (last_status_ && !retry_budget_.CanRetry()) {
            return NThreading::MakeFuture(MakeNonRetryableStatus());
        }

        result_.reset();
        const auto settings = MakeSingleAttemptSettings(deadline);
        // Budget the whole SDK attempt, including session acquisition failures.
        auto attempt = execute_attempt(
            client_,
            [handler = this->shared_from_this()](ArgType arg) {
                return handler->InvokeOperation(std::forward<ArgType>(arg));
            },
            settings
        );
        return attempt.Apply([handler = this->shared_from_this()](const NYdb::TAsyncStatus& status) {
            return handler->HandleAttempt(status);
        });
    }

    NYdb::TAsyncStatus InvokeOperation(ArgType arg) {
        const auto async_result = fn_(std::forward<ArgType>(arg));

        return async_result.Apply([handler = this->shared_from_this()](const auto& async_result) {
            return handler->HandleResult(async_result);
        });
    }

    NYdb::TStatus HandleResult(const AsyncResultType& async_result) {
        async_result.TryRethrow();

        // Alternatively, we could just copy the TFuture, causing pointless refcounting overhead.
        //
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
        result_.emplace(const_cast<AsyncResultType&>(async_result).ExtractValue());

        // NOLINTNEXTLINE(cppcoreguidelines-slicing)
        return NYdb::TStatus{*result_};
    }

    NYdb::TStatus HandleAttempt(const NYdb::TAsyncStatus& async_status) {
        last_status_ = async_status.GetValue();

        if (last_status_->IsSuccess()) {
            retry_budget_.AccountOk();
        } else if (IsRetryableStatus(last_status_->GetStatus())) {
            retry_budget_.AccountFail();
            if (!retry_budget_.CanRetry()) {
                return MakeNonRetryableStatus();
            }
        }

        return *last_status_;
    }

    ResultType TransformInternalRetryStatus(const NYdb::TAsyncStatus& internal_retry_status) {
        internal_retry_status.TryRethrow();

        if (!result_.has_value()) {
            throw YdbResponseError{
                "RetryOperation",
                last_status_ ? std::move(*last_status_) : NYdb::TStatus{internal_retry_status.GetValue()}
            };
        }

        return std::move(*result_);
    }

    TClient& client_;
    utils::RetryBudget& retry_budget_;
    NYdb::NRetry::TRetryOperationSettings retry_settings_;
    Fn fn_;

    std::optional<ResultType> result_;
    std::optional<NYdb::TStatus> last_status_;
};

// Fn: (NYdb::NTable::TSession) -> NThreading::TFuture<T>
//     OR
//     (NYdb::NTable::TTableClient&) -> NThreading::TFuture<T>
// RetryOperation -> NThreading::TFuture<T>
template <typename Fn>
auto RetryOperation(impl::RequestContext<OperationSettings>& request_context, Fn&& fn) {
    static_assert(std::is_invocable_v<Fn&, NYdb::NTable::TSession> || std::is_invocable_v<Fn&, NYdb::NTable::TTableClient&>);

    auto& client = request_context.connection.table_client;
    auto& retry_budget = request_context.connection.driver->GetRetryBudget();
    auto retry_handler = std::make_shared<RetryHandler<NYdb::NTable::TTableClient, Fn>>(
        client,
        retry_budget,
        PrepareRetrySettings(request_context.settings, retry_budget, request_context.deadline),
        std::forward<Fn>(fn)
    );
    return retry_handler->Execute();
}

// Fn: (NYdb::NQuery::TSession) -> NThreading::TFuture<T>
//     OR
//     (NYdb::NQuery::TQueryClient&) -> NThreading::TFuture<T>
// RetryQuery -> NThreading::TFuture<T>
template <typename Fn>
auto RetryQuery(impl::RequestContext<OperationSettings>& request_context, Fn&& fn) {
    static_assert(std::is_invocable_v<Fn&, NYdb::NQuery::TSession> || std::is_invocable_v<Fn&, NYdb::NQuery::TQueryClient&>);

    auto& client = request_context.connection.query_client;
    auto& retry_budget = request_context.connection.driver->GetRetryBudget();
    auto retry_handler = std::make_shared<RetryHandler<NYdb::NQuery::TQueryClient, Fn>>(
        client,
        retry_budget,
        PrepareRetrySettings(request_context.settings, retry_budget, request_context.deadline),
        std::forward<Fn>(fn)
    );
    return retry_handler->Execute();
}

}  // namespace ydb::impl

USERVER_NAMESPACE_END
