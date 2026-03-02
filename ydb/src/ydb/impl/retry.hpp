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
    std::chrono::milliseconds get_session_timeout_ms{5000};
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

template <typename TClient, typename Fn, typename GetSessionSettings>
class RetryHandler {
public:
    using TSession = typename TClient::TSession;

    RetryHandler(
        TClient& client,
        utils::RetryBudget& retry_budget,
        const CommonRetrySettings& retry_settings,
        Fn&& fn
    )
        : client_{client},
          retry_budget_{retry_budget},
          retry_settings_{retry_settings},
          get_session_settings_{GetSessionSettings().ClientTimeout(retry_settings_.get_session_timeout_ms)},
          fn_{std::move(fn)}
    {}

    void Execute() {
        std::optional<TSession> session;
        engine::Deadline deadline = engine::Deadline::FromDuration(retry_settings_.timeout_ms);
        for (std::uint32_t i = 0; i <= retry_settings_.retries && !deadline.IsReached(); ++i) {
            try {
                if constexpr (std::is_invocable_v<Fn&, TSession>) {
                    if (!session) {
                        auto get_session_future = client_.GetSession(get_session_settings_);
                        session = GetFutureValueChecked(std::move(get_session_future), "GetSession").GetSession();
                    }
                    fn_(*session);
                } else {
                    fn_(client_);
                }
                retry_budget_.AccountOk();
            } catch (const YdbResponseError& e) {
                auto [backoff, reset_session] = RetryStep::GetNext(retry_settings_, e.GetStatus().GetStatus(), i);
                if (reset_session) {
                    session.reset();
                }
                if (backoff.has_value()) {
                    retry_budget_.AccountFail();
                    engine::SleepUntil(std::min(engine::Deadline::FromDuration(*backoff), deadline));
                } else {
                    throw;
                }
            }
        }
    }

private:
    TClient& client_;
    utils::RetryBudget& retry_budget_;
    CommonRetrySettings retry_settings_;
    GetSessionSettings get_session_settings_;
    Fn fn_;
};

// Fn: (NYdb::NTable::TSession) -> void
//     OR
//     (NYdb::NTable::TTableClient&) -> void
// RetryOperation -> void
template <typename Fn>
void RetryOperation(impl::RequestContext<OperationSettings>& request_context, Fn&& fn) {
    static_assert(std::is_invocable_v<Fn&, NYdb::NTable::TSession> || std::is_invocable_v<Fn&, NYdb::NTable::TTableClient&>);

    auto& client = request_context.table_client.GetNativeTableClient();
    auto& retry_budget = request_context.table_client.GetRetryBudget();
    auto retry_handler = std::make_shared<RetryHandler<NYdb::NTable::TTableClient, Fn, NYdb::NTable::TCreateSessionSettings>>(
        client,
        retry_budget,
        PrepareRetrySettings<OperationSettings>(request_context.settings, retry_budget, request_context.deadline),
        std::forward<Fn>(fn)
    );
    retry_handler->Execute();
}

// Fn: (NYdb::NQuery::TSession) -> void
//     OR
//     (NYdb::NQuery::TQueryClient&) -> void
// RetryQuery -> void
template <typename Fn, typename Settings = OperationSettings>
void RetryQuery(impl::RequestContext<Settings>& request_context, Fn&& fn) {
    static_assert(std::is_invocable_v<Fn&, NYdb::NQuery::TSession> || std::is_invocable_v<Fn&, NYdb::NQuery::TQueryClient&>);

    auto& client = request_context.table_client.GetNativeQueryClient();
    auto& retry_budget = request_context.table_client.GetRetryBudget();
    auto retry_handler = std::make_shared<RetryHandler<NYdb::NQuery::TQueryClient, Fn, NYdb::NQuery::TCreateSessionSettings>>(
        client,
        retry_budget,
        PrepareRetrySettings<Settings>(request_context.settings, retry_budget, request_context.deadline),
        std::forward<Fn>(fn)
    );
    retry_handler->Execute();
}

}  // namespace ydb::impl

USERVER_NAMESPACE_END
