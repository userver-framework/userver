#pragma once

#include <memory>

#include <fmt/format.h>

#include <ydb-cpp-sdk/client/retry/retry.h>
#include <ydb-cpp-sdk/client/table/table.h>

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

template <typename Settings = OperationSettings>
CommonRetrySettings PrepareRetrySettings(
    const Settings& settings,
    const utils::RetryBudget& retry_budget,
    engine::Deadline deadline
);

struct RetryStep {
    static RetryStep GetNext(const CommonRetrySettings& retry_settings, NYdb::EStatus status, std::uint32_t retry_number);

    std::optional<std::chrono::milliseconds> backoff = std::nullopt;
    bool reset_session = false;
};

template <typename... Args>
NYdb::TAsyncStatus RetryOperation(NYdb::NTable::TTableClient& table_client, Args&&... args) {
    return table_client.RetryOperation(std::forward<Args>(args)...);
}

template <typename... Args>
NYdb::TAsyncStatus RetryOperation(NYdb::NQuery::TQueryClient& query_client, Args&&... args) {
    return query_client.RetryQuery(std::forward<Args>(args)...);
}

template <typename TClient, typename Settings, typename Fn, typename GetSessionSettings>
class RetryHandler {
public:
    using TSession = typename TClient::TSession;

    RetryHandler(
        TClient& native_client,
        TableClient& table_client,
        const Settings& settings,
        engine::Deadline deadline,
        Fn&& fn
    )
        : native_client_{native_client},
          table_client_{table_client},
          settings_{settings},
          retry_settings_{PrepareRetrySettings<Settings>(settings_, table_client_.GetRetryBudget(), deadline)},
          fn_{std::move(fn)},
          deadline_{engine::Deadline::FromDuration(retry_settings_.timeout_ms)}
    {}

    void Execute() {
        for (std::uint32_t i = 0; i <= retry_settings_.retries && !deadline_.IsReached(); ++i) {
            auto backoff_until = TryExecute(i);
            if (!backoff_until.IsReached()) {
                engine::SleepUntil(std::min(backoff_until, deadline_));
            }
        }
    }

    engine::Deadline TryExecute(std::uint32_t retry_number) {
        //RequestContext attempt_context{retry_context_, Query{"", Query::Name{"ydb.Attempt"}}, IsInternalContext{true}};

        try {
            if constexpr (std::is_invocable_v<Fn&, TClient&>) {
                fn_(native_client_);
            } else {
                if (!session_) {
                    RequestContext<Settings> get_session_context{table_client_, Query{"", Query::Name{"GetSession"}}, Settings{settings_}, IsStreaming{false}, nullptr, deadline_};
                    auto get_session_settings = PrepareRequestSettings<GetSessionSettings>(get_session_context.settings, get_session_context.deadline);

                    auto get_session_future = native_client_.GetSession(get_session_settings);
                    session_ = GetFutureValueChecked(std::move(get_session_future), "GetSession", get_session_context).GetSession();
                }
                if constexpr (std::is_invocable_v<Fn&, TSession, engine::Deadline>) {
                    fn_(*session_, deadline_);
                } else {
                    fn_(*session_);
                }
            }

            table_client_.GetRetryBudget().AccountOk();
            return engine::Deadline::Passed();
        } catch (const YdbResponseError& e) {
            table_client_.GetRetryBudget().AccountFail();

            auto [backoff, reset_session] = RetryStep::GetNext(retry_settings_, e.GetStatus().GetStatus(), retry_number);
            if (reset_session) {
                session_.reset();
            }
            if (!backoff.has_value()) {
                throw;
            }

            return engine::Deadline::FromDuration(*backoff);
        }
    }

private:
    TClient& native_client_;
    TableClient& table_client_;
    Settings settings_;
    CommonRetrySettings retry_settings_;
    Fn fn_;
    engine::Deadline deadline_;

    std::optional<TSession> session_;
};

// Fn: (NYdb::NTable::TSession) -> void
//     OR
//     (NYdb::NTable::TTableClient&) -> void
// RetryOperation -> void
template <typename Fn>
void RetryOperation(impl::RequestContext<OperationSettings>& request_context, Fn&& fn) {
    static_assert(std::is_invocable_v<Fn&, NYdb::NTable::TSession> || std::is_invocable_v<Fn&, NYdb::NTable::TTableClient&>);

    try {
        RetryHandler<NYdb::NTable::TTableClient, OperationSettings, Fn, NYdb::NTable::TCreateSessionSettings>(
            request_context.table_client.GetNativeTableClient(),
            request_context.table_client,
            request_context.settings,
            request_context.deadline,
            std::forward<Fn>(fn)
        ).Execute();
    } catch (const YdbResponseError& e) {
        request_context.HandleError(e.GetStatus());
        throw;
    }
}

// Fn: (NYdb::NQuery::TSession) -> void
//     OR
//     (NYdb::NQuery::TQueryClient&) -> void
// RetryQuery -> void
template <typename Fn>
void RetryQuery(impl::RequestContext<OperationSettings>& request_context, Fn&& fn) {
    static_assert(std::is_invocable_v<Fn&, NYdb::NQuery::TSession> || std::is_invocable_v<Fn&, NYdb::NQuery::TQueryClient&>);

    try {
        RetryHandler<NYdb::NQuery::TQueryClient, OperationSettings, Fn, NYdb::NQuery::TCreateSessionSettings>(
            request_context.table_client.GetNativeQueryClient(),
            request_context.table_client,
            request_context.settings,
            request_context.deadline,
            std::forward<Fn>(fn)
        ).Execute();
    } catch (const YdbResponseError& e) {
        request_context.HandleError(e.GetStatus());
        throw;
    }
}

template <typename Fn>
void RetryQuery(const RetryTxSettings& retry_settings, TableClient& table_client, engine::Deadline deadline, Fn&& fn) {
    static_assert(std::is_invocable_v<Fn&, NYdb::NQuery::TSession> || std::is_invocable_v<Fn&, NYdb::NQuery::TSession, engine::Deadline>);

    RetryHandler<NYdb::NQuery::TQueryClient, RetryTxSettings, Fn, NYdb::NQuery::TCreateSessionSettings>(
        table_client.GetNativeQueryClient(),
        table_client,
        retry_settings,
        deadline,
        std::forward<Fn>(fn)
    ).Execute();
}

}  // namespace ydb::impl

USERVER_NAMESPACE_END
