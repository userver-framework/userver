#include "test_utils.hpp"

#include <chrono>
#include <cstddef>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>

#include <ydb-cpp-sdk/library/issue/yql_issue.h>

#include <userver/engine/single_consumer_event.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/utils/async.hpp>
#include <userver/utils/statistics/testing.hpp>
#include <userver/ydb/exceptions.hpp>
#include <userver/ydb/table.hpp>
#include <userver/ydb/transaction.hpp>

#include <userver/utils/retry_budget.hpp>

#include <ydb/impl/future.hpp>
#include <ydb/impl/request_context.hpp>
#include <ydb/impl/retry_tx.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

class RetryTxFixture : public ydb::ClientFixtureBase {
public:
    template <typename Func>
    void RetryTx(std::size_t retries, Func func) {
        auto settings = MakeRetryTxSettings(retries);

        auto callback =
            [func = std::move(func)](ydb::impl::RetryTxSession& session, engine::Deadline deadline) mutable {
                func(session.GetSession(), deadline);
            };
        ydb::impl::RetryTx(settings, GetTableClient(), engine::Deadline{}, std::move(callback));
    }

private:
    ydb::RetryTxSettings MakeRetryTxSettings(std::uint32_t retries) {
        static constexpr std::chrono::milliseconds kTimeout{3000};
        return ydb::RetryTxSettings{
            .timeout_ms = kTimeout,
            .retries = retries,
            .get_session_settings =
                ydb::GetSessionSettings{
                    .client_timeout_ms = kTimeout,
                },
            .commit_settings =
                ydb::CommitSettings{
                    .client_timeout_ms = kTimeout,
                },
            .rollback_settings =
                ydb::RollbackSettings{
                    .client_timeout_ms = kTimeout,
                },
        };
    }
};

// NOLINTNEXTLINE(fuchsia-multiple-inheritance)
class RetryTxSingleSessionFixture : public ydb::TestClientsBase, public ::testing::Test {
public:
    RetryTxSingleSessionFixture() { InitializeClients(); }

protected:
    ydb::impl::TableSettings GetTableSettings() const override { return {.min_pool_size = 1, .max_pool_size = 1}; }

    std::string GetSessionId() {
        auto result = ydb::impl::GetFutureValueChecked(GetNativeQueryClient().GetSession(), "GetSession");
        return result.GetSession().GetId();
    }

    void DeleteSession(const std::string& session_id) {
        const auto status = ydb::impl::GetFutureValueUnchecked(GetNativeQueryClient().DeleteSession(session_id));
        EXPECT_TRUE(status.IsSuccess());
    }
};

constexpr NYdb::EStatus kRetryableStatus = NYdb::EStatus::ABORTED;
constexpr NYdb::EStatus kNonRetryableStatus = NYdb::EStatus::BAD_REQUEST;
constexpr auto kRequestTimeout = std::chrono::milliseconds{300};
constexpr auto kSessionTimeout = std::chrono::milliseconds{1500};
constexpr auto kRollbackDeadlineTimeout = std::chrono::seconds{2};
constexpr auto kEmptyCallbackTimeout = std::chrono::milliseconds{100};
const ydb::Query kStartTransactionQuery{"SELECT 1;"};

class CallbackPreconditionError final : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

inline void MakeErrorResponse(NYdb::EStatus status) {
    throw ydb::YdbResponseError{"", NYdb::TStatus(status, NYdb::NIssue::TIssues{})};
}

}  // namespace

TEST(RetryTxSettings, SeparateSessionTimeout) {
    const ydb::OperationSettings defaults{
        .client_timeout_ms = kRequestTimeout,
        .get_session_timeout_ms = kSessionTimeout,
    };
    ydb::RetryTxSettings settings;
    ydb::impl::PrepareSettings(settings, defaults);
    EXPECT_EQ(settings.get_session_settings.client_timeout_ms, kSessionTimeout);
    EXPECT_FALSE(settings.commit_settings.client_timeout_ms.has_value());
    EXPECT_FALSE(settings.rollback_settings.client_timeout_ms.has_value());
}

TEST(RetryTxSettings, ExplicitSessionTimeout) {
    const ydb::OperationSettings defaults{
        .client_timeout_ms = kRequestTimeout,
        .get_session_timeout_ms = kSessionTimeout,
    };
    for (const auto timeout : {std::chrono::milliseconds::zero(), kRequestTimeout}) {
        ydb::RetryTxSettings settings{.get_session_settings = {.client_timeout_ms = timeout}};
        ydb::impl::PrepareSettings(settings, defaults);
        EXPECT_EQ(settings.get_session_settings.client_timeout_ms, timeout);
    }
}

UTEST_F(RetryTxFixture, WaitsForBusySessionBeyondRequestTimeout) {
    const auto borrowed_driver = std::shared_ptr<ydb::impl::Driver>{&GetDriver(), [](ydb::impl::Driver*) {}};
    ydb::TableClient client{
        ydb::impl::TableSettings{.min_pool_size = 1, .max_pool_size = 1},
        ydb::OperationSettings{
            .retries = 0,
            .client_timeout_ms = kRequestTimeout,
            .tx_mode = ydb::TransactionMode::kSerializableRW,
            .get_session_timeout_ms = kSessionTimeout,
        },
        dynamic_config::GetDefaultSource(),
        borrowed_driver,
    };
    engine::SingleConsumerEvent started;
    engine::TaskWithResult<void> waiting;
    {
        const auto held_session =
            ydb::impl::GetFutureValueChecked(client.GetNativeQueryClient().GetSession(), "GetSession").GetSession();
        ASSERT_EQ(client.GetNativeQueryClient().GetActiveSessionCount(), 1);
        waiting = utils::Async("wait_for_session", [&client, &started] {
            started.Send();
            client.RetryTx("wait_for_session", {}, [](ydb::TxActor&) { return ydb::TxAction::kCommit; });
        });
        ASSERT_TRUE(started.WaitForEvent());
        engine::SleepFor(std::chrono::milliseconds{700});
    }
    UASSERT_NO_THROW(waiting.Get());
}

UTEST_F(RetryTxFixture, Success) {
    std::size_t attempts = 0;
    ASSERT_NO_THROW(RetryTx(/*retries=*/3, [&attempts](NYdb::NQuery::TSession, engine::Deadline) { attempts++; }));

    ASSERT_EQ(attempts, 1);
};

UTEST_F(RetryTxFixture, DeadlineBeforeFirstAttemptIsAnError) {
    std::size_t attempts = 0;
    auto callback = [&attempts](ydb::impl::RetryTxSession&, engine::Deadline) { ++attempts; };
    const ydb::RetryTxSettings settings{.timeout_ms = std::chrono::milliseconds{1}, .retries = 0};
    ydb::impl::RetryTxHandler
        handler{GetNativeQueryClient(), GetTableClient(), settings, engine::Deadline{}, std::move(callback)};
    engine::SleepFor(std::chrono::milliseconds{10});
    EXPECT_THROW(handler.Execute(), ydb::DeadlineExceededError);
    EXPECT_EQ(attempts, 0);
}

UTEST_F(RetryTxFixture, NonRetry) {
    std::size_t attempts = 0;
    try {
        RetryTx(/*retries=*/3, [&attempts](NYdb::NQuery::TSession, engine::Deadline) {
            attempts++;
            MakeErrorResponse(kNonRetryableStatus);
        });
    } catch (const ydb::YdbResponseError& e) {
        ASSERT_EQ(e.GetStatus().GetStatus(), kNonRetryableStatus);
        ASSERT_EQ(attempts, 1);
        return;
    }
    FAIL() << "Expected YdbResponseError";
};

UTEST_F(RetryTxFixture, SuccessOnTheLastAttempt) {
    constexpr std::uint32_t kRetries = 5;
    std::size_t attempts = 0;
    ASSERT_NO_THROW(RetryTx(/*retries=*/kRetries, [&attempts](NYdb::NQuery::TSession, engine::Deadline) {
        attempts++;
        if (attempts < kRetries) {
            MakeErrorResponse(kRetryableStatus);
        }
    }));

    ASSERT_EQ(attempts, kRetries);
};

UTEST_F(RetryTxFixture, AttemptsIsRetriesPlusOne) {
    constexpr std::uint32_t kRetries = 5;
    std::size_t attempts = 0;
    try {
        RetryTx(
            /*retries=*/kRetries,
            [&attempts](NYdb::NQuery::TSession, engine::Deadline) {
                attempts++;
                MakeErrorResponse(kRetryableStatus);
            }
        );
    } catch (const ydb::YdbResponseError& e) {
        ASSERT_EQ(e.GetStatus().GetStatus(), kRetryableStatus);
        ASSERT_EQ(attempts, kRetries + 1);
        return;
    }
    FAIL() << "Expected YdbResponseError";
};

UTEST_F(RetryTxFixture, NoBackoffAfterLastAttempt) {
    constexpr auto kMaxPropagationTime = std::chrono::milliseconds{300};
    for (const auto retries : {0U, 1U}) {
        SCOPED_TRACE(retries);
        std::size_t attempts = 0;
        std::optional<std::chrono::steady_clock::time_point> failed_at;
        try {
            RetryTx(retries, [&attempts, &failed_at, retries](NYdb::NQuery::TSession, engine::Deadline) {
                ++attempts;
                if (attempts <= retries) {
                    MakeErrorResponse(kRetryableStatus);
                }
                failed_at = std::chrono::steady_clock::now();
                MakeErrorResponse(NYdb::EStatus::OVERLOADED);
            });
            FAIL() << "Expected YdbResponseError";
        } catch (const ydb::YdbResponseError& error) {
            EXPECT_EQ(error.GetStatus().GetStatus(), NYdb::EStatus::OVERLOADED);
        }
        ASSERT_EQ(attempts, retries + 1);
        ASSERT_TRUE(failed_at.has_value());
        EXPECT_LT(std::chrono::steady_clock::now() - *failed_at, kMaxPropagationTime);
    }
}

UTEST_F(RetryTxFixture, Exception) {
    UASSERT_THROW_MSG(
        RetryTx(
            /*retries=*/0,
            [](NYdb::NQuery::TSession, engine::Deadline) { throw std::runtime_error{"error"}; }
        ),
        std::runtime_error,
        "error"
    );
};

UTEST_F(RetryTxFixture, RetryBudgetExhaustionAndRecovery) {
    constexpr std::size_t kBudgetCapacity = 6;
    constexpr std::size_t kAttemptsUntilExhausted = 2;
    constexpr std::size_t kMaxRetries = 5;
    static_assert(kAttemptsUntilExhausted > 0 && kAttemptsUntilExhausted <= kBudgetCapacity / 2);
    constexpr std::size_t kInitialTokens = kBudgetCapacity / 2 + kAttemptsUntilExhausted;
    auto& budget = GetTableClient().GetRetryBudget();
    // Refill tokens spent during client startup, then use one token per success.
    budget.SetSettings({.max_tokens = kBudgetCapacity, .token_ratio = kBudgetCapacity});
    budget.AccountOk();
    budget.SetSettings({.max_tokens = kBudgetCapacity, .token_ratio = 1});
    for (std::size_t i = 0; i + kInitialTokens < kBudgetCapacity; ++i) {
        budget.AccountFail();
    }
    ASSERT_TRUE(budget.CanRetry());

    std::size_t attempts = 0;
    try {
        RetryTx(kMaxRetries, [&](NYdb::NQuery::TSession, engine::Deadline) {
            ++attempts;
            MakeErrorResponse(kRetryableStatus);
        });
        FAIL() << "Expected YdbResponseError";
    } catch (const ydb::YdbResponseError& error) {
        EXPECT_EQ(error.GetStatus().GetStatus(), kRetryableStatus);
    }
    EXPECT_EQ(attempts, kAttemptsUntilExhausted);
    EXPECT_FALSE(budget.CanRetry());

    attempts = 0;
    ASSERT_NO_THROW(RetryTx(kMaxRetries, [&](NYdb::NQuery::TSession, engine::Deadline) { ++attempts; }));
    EXPECT_EQ(attempts, 1);
    EXPECT_TRUE(budget.CanRetry());
    budget.AccountFail();
    EXPECT_FALSE(budget.CanRetry());
}

UTEST_F(RetryTxFixture, AbortedCallbackRetriesWithoutRollback) {
    std::size_t attempts = 0;
    GetTableClient().RetryTx("retry_tx_aborted", ydb::RetryTxSettings{.retries = 1}, [&attempts](ydb::TxActor&) {
        if (++attempts == 1) {
            MakeErrorResponse(kRetryableStatus);
        }
        return ydb::TxAction::kCommit;
    });

    EXPECT_EQ(attempts, 2);
    EXPECT_FALSE(GetMetrics().SingleMetricOptional("ydb.by-query.total", {{"ydb_query", "Rollback"}}).has_value());
    EXPECT_EQ(GetMetrics().SingleMetric("ydb.by-query.total", {{"ydb_query", "GetSession"}}).AsRate().value, 1);
}

UTEST_F(RetryTxFixture, NonRetryableCallbackRollsBackAndRethrows) {
    const ydb::YdbResponseError error{"retry_tx_error", NYdb::TStatus{kNonRetryableStatus, NYdb::NIssue::TIssues{}}};
    std::size_t attempts = 0;
    try {
        GetTableClient().RetryTx(
            "retry_tx_error",
            ydb::RetryTxSettings{.retries = 1},
            [&attempts, &error](ydb::TxActor& tx) -> ydb::TxAction {
                ++attempts;
                tx.Execute(ydb::Query{"SELECT 1;"});
                throw ydb::YdbResponseError{error};
            }
        );
        FAIL() << "Expected YdbResponseError";
    } catch (const ydb::YdbResponseError& e) {
        EXPECT_EQ(e.GetStatus().GetStatus(), kNonRetryableStatus);
        EXPECT_STREQ(e.what(), error.what());
    }

    EXPECT_EQ(attempts, 1);
    EXPECT_EQ(GetMetrics().SingleMetric("ydb.by-query.success", {{"ydb_query", "Rollback"}}).AsRate().value, 1);
}

UTEST_F(RetryTxSingleSessionFixture, ReplacesLostSessionBeforeTransactionStarts) {
    for (const auto callback_status : {NYdb::EStatus::UNAVAILABLE, NYdb::EStatus::OVERLOADED}) {
        SCOPED_TRACE(static_cast<int>(callback_status));
        const auto session_id = GetSessionId();
        std::size_t attempts = 0;
        UASSERT_NO_THROW(GetTableClient().RetryTx(
            "lost_session_before_transaction",
            ydb::RetryTxSettings{.retries = 1},
            [this, &attempts, &session_id, callback_status](ydb::TxActor& tx) {
                if (++attempts == 1) {
                    DeleteSession(session_id);
                    EXPECT_THROW(tx.Execute(kStartTransactionQuery), ydb::YdbResponseError);
                    MakeErrorResponse(callback_status);
                }
                tx.Execute(kStartTransactionQuery);
                return ydb::TxAction::kCommit;
            }
        ));
        EXPECT_EQ(attempts, 2);
    }
    EXPECT_FALSE(GetMetrics().SingleMetricOptional("ydb.by-query.total", {{"ydb_query", "Rollback"}}).has_value());
    EXPECT_EQ(GetMetrics().SingleMetric("ydb.by-query.total", {{"ydb_query", "GetSession"}}).AsRate().value, 4);
}

UTEST_F(RetryTxSingleSessionFixture, ReplacesLostSessionAfterRollbackError) {
    for (const auto callback_status : {NYdb::EStatus::UNAVAILABLE, NYdb::EStatus::OVERLOADED}) {
        SCOPED_TRACE(static_cast<int>(callback_status));
        const auto session_id = GetSessionId();
        std::size_t attempts = 0;
        UASSERT_NO_THROW(GetTableClient().RetryTx(
            "lost_session",
            ydb::RetryTxSettings{.retries = 1},
            [this, &attempts, &session_id, callback_status](ydb::TxActor& tx) {
                tx.Execute(kStartTransactionQuery);
                if (++attempts == 1) {
                    DeleteSession(session_id);
                    MakeErrorResponse(callback_status);
                }
                return ydb::TxAction::kCommit;
            }
        ));
        EXPECT_EQ(attempts, 2);
    }
    EXPECT_EQ(GetMetrics().SingleMetric("ydb.by-query.error", {{"ydb_query", "Rollback"}}).AsRate().value, 2);
    EXPECT_EQ(GetMetrics().SingleMetric("ydb.by-query.total", {{"ydb_query", "GetSession"}}).AsRate().value, 4);
}

UTEST_F(RetryTxSingleSessionFixture, NonRetryableCallbackErrorSurvivesLostSession) {
    const auto session_id = GetSessionId();
    const ydb::YdbResponseError error{"callback_error", NYdb::TStatus{kNonRetryableStatus, NYdb::NIssue::TIssues{}}};
    std::size_t attempts = 0;
    try {
        GetTableClient().RetryTx(
            "lost_session_callback_error",
            ydb::RetryTxSettings{.retries = 1},
            [this, &attempts, &session_id, &error](ydb::TxActor& tx) -> ydb::TxAction {
                ++attempts;
                tx.Execute(kStartTransactionQuery);
                DeleteSession(session_id);
                throw ydb::YdbResponseError{error};
            }
        );
        FAIL() << "Expected the callback error";
    } catch (const ydb::YdbResponseError& actual) {
        EXPECT_EQ(actual.GetStatus().GetStatus(), kNonRetryableStatus);
        EXPECT_STREQ(actual.what(), error.what());
    }
    EXPECT_EQ(attempts, 1);
    EXPECT_EQ(GetMetrics().SingleMetric("ydb.by-query.error", {{"ydb_query", "Rollback"}}).AsRate().value, 1);
    EXPECT_EQ(GetMetrics().SingleMetric("ydb.by-query.total", {{"ydb_query", "GetSession"}}).AsRate().value, 1);
}

UTEST_F(RetryTxFixture, CallbackErrorSurvivesRollbackDeadline) {
    std::size_t attempts = 0;
    UASSERT_THROW_MSG(
        GetTableClient().RetryTx(
            "callback_error",
            ydb::RetryTxSettings{.timeout_ms = kRollbackDeadlineTimeout, .retries = 1},
            [&attempts](ydb::TxActor& tx) -> ydb::TxAction {
                ++attempts;
                tx.Execute(kStartTransactionQuery);
                engine::SleepFor(kRollbackDeadlineTimeout);
                throw CallbackPreconditionError{"precondition failed"};
            }
        ),
        CallbackPreconditionError,
        "precondition failed"
    );
    EXPECT_EQ(attempts, 1);
}

UTEST_F(RetryTxFixture, YdbCallbackErrorSurvivesRollbackDeadline) {
    const ydb::YdbResponseError error{"callback_error", NYdb::TStatus{kNonRetryableStatus, NYdb::NIssue::TIssues{}}};
    std::size_t attempts = 0;
    try {
        GetTableClient().RetryTx(
            "ydb_callback_error",
            ydb::RetryTxSettings{.timeout_ms = kRollbackDeadlineTimeout, .retries = 1},
            [&attempts, &error](ydb::TxActor& tx) -> ydb::TxAction {
                ++attempts;
                tx.Execute(kStartTransactionQuery);
                engine::SleepFor(kRollbackDeadlineTimeout);
                throw ydb::YdbResponseError{error};
            }
        );
        FAIL() << "Expected YdbResponseError";
    } catch (const ydb::YdbResponseError& actual) {
        EXPECT_EQ(actual.GetStatus().GetStatus(), kNonRetryableStatus);
        EXPECT_STREQ(actual.what(), error.what());
    }
    EXPECT_EQ(attempts, 1);
}

UTEST_F(RetryTxFixture, ExplicitRollbackReportsDeadline) {
    EXPECT_THROW(
        GetTableClient().RetryTx(
            "explicit_rollback",
            ydb::RetryTxSettings{.timeout_ms = kRollbackDeadlineTimeout, .retries = 0},
            [](ydb::TxActor& tx) {
                tx.Execute(kStartTransactionQuery);
                engine::SleepFor(kRollbackDeadlineTimeout);
                return ydb::TxAction::kRollback;
            }
        ),
        ydb::DeadlineExceededError
    );
}

UTEST_F(RetryTxFixture, EmptyCallbackDoesNotStartTransaction) {
    for (const auto action : {ydb::TxAction::kCommit, ydb::TxAction::kRollback}) {
        GetTableClient().RetryTx("empty_transaction", {}, [action](ydb::TxActor&) { return action; });
    }
    const auto metrics = GetMetrics();
    EXPECT_FALSE(metrics.SingleMetricOptional("ydb.by-query.total", {{"ydb_query", "Begin"}}).has_value());
    EXPECT_FALSE(metrics.SingleMetricOptional("ydb.by-query.total", {{"ydb_query", "Commit"}}).has_value());
    EXPECT_FALSE(metrics.SingleMetricOptional("ydb.by-query.total", {{"ydb_query", "Rollback"}}).has_value());
}

UTEST_F(RetryTxFixture, EmptyCallbackHonorsDeadline) {
    for (const auto action : {ydb::TxAction::kCommit, ydb::TxAction::kRollback}) {
        SCOPED_TRACE(action == ydb::TxAction::kCommit ? "commit" : "rollback");
        std::size_t calls = 0;
        EXPECT_THROW(
            GetTableClient().RetryTx(
                "empty_transaction_timeout",
                ydb::RetryTxSettings{.timeout_ms = kEmptyCallbackTimeout, .retries = 0},
                [&calls, action](ydb::TxActor&) {
                    ++calls;
                    engine::SleepFor(kEmptyCallbackTimeout * 2);
                    return action;
                }
            ),
            ydb::DeadlineExceededError
        );
        EXPECT_EQ(calls, 1);
    }
    const auto metrics = GetMetrics();
    EXPECT_FALSE(metrics.SingleMetricOptional("ydb.by-query.total", {{"ydb_query", "Begin"}}).has_value());
    EXPECT_FALSE(metrics.SingleMetricOptional("ydb.by-query.total", {{"ydb_query", "Commit"}}).has_value());
    EXPECT_FALSE(metrics.SingleMetricOptional("ydb.by-query.total", {{"ydb_query", "Rollback"}}).has_value());
}

UTEST_F(RetryTxFixture, EmptyCallbackErrorSurvivesDeadline) {
    std::size_t calls = 0;
    UASSERT_THROW_MSG(
        GetTableClient().RetryTx(
            "empty_transaction_error",
            ydb::RetryTxSettings{.timeout_ms = kEmptyCallbackTimeout, .retries = 1},
            [&calls](ydb::TxActor&) -> ydb::TxAction {
                ++calls;
                engine::SleepFor(kEmptyCallbackTimeout * 2);
                throw CallbackPreconditionError{"callback failed"};
            }
        ),
        CallbackPreconditionError,
        "callback failed"
    );
    EXPECT_EQ(calls, 1);
}

USERVER_NAMESPACE_END
