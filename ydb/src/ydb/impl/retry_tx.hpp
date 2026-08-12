#pragma once

#include <memory>

#include <fmt/format.h>

#include <ydb-cpp-sdk/client/query/client.h>
#include <ydb-cpp-sdk/client/retry/retry.h>

#include <userver/engine/sleep.hpp>
#include <userver/utils/retry_budget.hpp>
#include <userver/ydb/exceptions.hpp>

#include <ydb/impl/future.hpp>
#include <ydb/impl/request_context.hpp>

USERVER_NAMESPACE_BEGIN

namespace ydb::impl {

struct BackoffSettings {
    std::chrono::milliseconds slot_duration_ms{1000};
    std::uint32_t ceiling{6};
    double uncertain_ratio{0.5};
};

struct CommonRetrySettings {
    std::chrono::milliseconds timeout_ms{std::chrono::milliseconds::max()};
    std::uint32_t retries{10};
    bool is_idempotent{false};

    BackoffSettings fast_backoff_settings{
        .slot_duration_ms = std::chrono::milliseconds(5),
        .ceiling = 10,
        .uncertain_ratio = 0.5,
    };

    BackoffSettings slow_backoff_settings{
        .slot_duration_ms = std::chrono::seconds(1),
        .ceiling = 6,
        .uncertain_ratio = 0.5,
    };
};

CommonRetrySettings PrepareRetrySettings(
    const RetryTxSettings& settings,
    const utils::RetryBudget& retry_budget,
    engine::Deadline deadline
);

struct RetryStep {
    static RetryStep GetNext(
        const CommonRetrySettings& retry_settings,
        NYdb::EStatus status,
        std::uint32_t retry_number
    );

    std::optional<std::chrono::milliseconds> backoff = std::nullopt;
    bool reset_session = false;
};

template <typename Fn>
class RetryTxHandler {
public:
    RetryTxHandler(
        NYdb::NQuery::TQueryClient& native_client,
        TableClient& table_client,
        const RetryTxSettings& settings,
        engine::Deadline deadline,
        Fn&& fn
    )
        : native_client_{native_client},
          table_client_{table_client},
          settings_{settings},
          retry_settings_{PrepareRetrySettings(settings_, table_client_.GetRetryBudget(), deadline)},
          deadline_{engine::Deadline::FromDuration(retry_settings_.timeout_ms)},
          fn_{std::move(fn)}
    {}

    void Execute() {
        std::exception_ptr exception;
        for (std::uint32_t i = 0; i <= retry_settings_.retries && !deadline_.IsReached(); ++i) {
            engine::Deadline backoff_until;
            std::tie(backoff_until, exception) = TryExecute(i);

            if (exception) {
                engine::InterruptibleSleepUntil(std::min(backoff_until, deadline_));
            } else {
                break;
            }

            if (engine::current_task::ShouldCancel()) {
                throw OperationCancelledError();
            }
        }
        if (exception) {
            std::rethrow_exception(exception);
        }
    }

    std::pair<engine::Deadline, std::exception_ptr> TryExecute(std::uint32_t retry_number) {
        try {
            if (!session_) {
                UpdateSession(retry_number);
            }

            fn_(*session_, deadline_);

            table_client_.GetRetryBudget().AccountOk();
            return {engine::Deadline::Passed(), std::exception_ptr{}};
        } catch (const YdbResponseError& e) {
            table_client_.GetRetryBudget().AccountFail();

            auto
                [backoff, reset_session] = RetryStep::GetNext(retry_settings_, e.GetStatus().GetStatus(), retry_number);
            if (reset_session) {
                session_.reset();
            }
            if (!backoff.has_value()) {
                throw;
            }

            return {engine::Deadline::FromDuration(*backoff), std::current_exception()};
        }
    }

    void UpdateSession(std::uint32_t retry_number) {
        auto get_session_context = table_client_.MakeRequestContext(
            Query{"", Query::Name{"GetSession"}},
            GetSessionSettings{settings_.get_session_settings},
            IsStreaming{false},
            nullptr,
            deadline_
        );

        get_session_context.span.AddTag("attempt", retry_number + 1);

        auto get_session_settings = PrepareRequestSettings<NYdb::NQuery::TCreateSessionSettings>(
            get_session_context.settings,
            get_session_context.deadline,
            get_session_context.span.GetTraceId()
        );

        auto get_session_future = native_client_.GetSession(get_session_settings);
        session_ =
            impl::GetFutureValueChecked(std::move(get_session_future), "GetSession", get_session_context).GetSession();
    }

private:
    NYdb::NQuery::TQueryClient& native_client_;
    TableClient& table_client_;
    RetryTxSettings settings_;
    CommonRetrySettings retry_settings_;
    engine::Deadline deadline_;
    Fn fn_;

    std::optional<NYdb::NQuery::TSession> session_;
};

template <typename Fn>
void RetryTx(const RetryTxSettings& retry_settings, TableClient& table_client, engine::Deadline deadline, Fn&& fn) {
    static_assert(std::is_invocable_v<Fn&, NYdb::NQuery::TSession, engine::Deadline>);

    RetryTxHandler<
        Fn>(table_client.GetNativeQueryClient(), table_client, retry_settings, deadline, std::forward<Fn>(fn))
        .Execute();
}

}  // namespace ydb::impl

USERVER_NAMESPACE_END
