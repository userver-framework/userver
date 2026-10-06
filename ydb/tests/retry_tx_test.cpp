#include "test_utils.hpp"

#include <cstddef>

#include <ydb-cpp-sdk/library/issue/yql_issue.h>

#include <userver/utils/statistics/testing.hpp>
#include <userver/ydb/exceptions.hpp>
#include <userver/ydb/table.hpp>
#include <userver/ydb/transaction.hpp>

#include <userver/utils/retry_budget.hpp>

#include <ydb/impl/future.hpp>
#include <ydb/impl/retry_tx.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

class RetryTxFixture : public ydb::ClientFixtureBase {
public:
    template <typename Func>
    void RetryTx(std::size_t retries, Func func) {
        auto settings = MakeRetryTxSettings(retries);

        ydb::impl::RetryTx(settings, GetTableClient(), engine::Deadline{}, std::move(func));
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

constexpr NYdb::EStatus kRetryableStatus = NYdb::EStatus::ABORTED;
constexpr NYdb::EStatus kNonRetryableStatus = NYdb::EStatus::BAD_REQUEST;

inline void MakeErrorResponse(NYdb::EStatus status) {
    throw ydb::YdbResponseError{"", NYdb::TStatus(status, NYdb::NIssue::TIssues{})};
}

}  // namespace

UTEST_F(RetryTxFixture, Success) {
    std::size_t attempts = 0;
    ASSERT_NO_THROW(RetryTx(/*retries=*/3, [&attempts](NYdb::NQuery::TSession, engine::Deadline) { attempts++; }));

    ASSERT_EQ(attempts, 1);
};

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
}

UTEST_F(RetryTxFixture, NonRetryableCallbackRollsBackAndRethrows) {
    const ydb::YdbResponseError error{"retry_tx_error", NYdb::TStatus{kNonRetryableStatus, NYdb::NIssue::TIssues{}}};
    std::size_t attempts = 0;
    try {
        GetTableClient().RetryTx(
            "retry_tx_error",
            ydb::RetryTxSettings{.retries = 1},
            [&attempts, &error](ydb::TxActor&) -> ydb::TxAction {
                ++attempts;
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

USERVER_NAMESPACE_END
