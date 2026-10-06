#include "test_utils.hpp"

#include <chrono>
#include <cstddef>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>

#include <library/cpp/threading/future/core/future.h>
#include <util/datetime/base.h>
#include <ydb-cpp-sdk/client/query/client.h>
#include <ydb-cpp-sdk/client/retry/retry.h>
#include <ydb-cpp-sdk/client/table/table.h>
#include <ydb-cpp-sdk/library/issue/yql_issue.h>

#include <userver/engine/sleep.hpp>
#include <userver/utest/utest.hpp>
#include <userver/utils/retry_budget.hpp>

#include <ydb/impl/future.hpp>
#include <ydb/impl/retry.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

class RetryOperationFixture : public ydb::ClientFixtureBase {
public:
    template <typename Func>
    auto RetryOperationSync(std::size_t retries, Func func) {
        auto settings = MakeOperationSettings(retries);
        auto context = GetTableClient().MakeRequestContext(ydb::Query{}, std::move(settings));

        auto future = ydb::impl::RetryOperation(context, std::move(func));

        return ydb::impl::GetFutureValueUnchecked(std::move(future));
    }

private:
    ydb::OperationSettings MakeOperationSettings(std::uint32_t retries) {
        static constexpr std::chrono::milliseconds kTimeout{3000};
        return ydb::OperationSettings{
            .retries = retries,
            .client_timeout_ms = kTimeout,
            .get_session_timeout_ms = kTimeout,
        };
    }
};

constexpr NYdb::EStatus kSuccess = NYdb::EStatus::SUCCESS;
constexpr NYdb::EStatus kRetryableStatus = NYdb::EStatus::ABORTED;
constexpr NYdb::EStatus kNonRetryableStatus = NYdb::EStatus::BAD_REQUEST;

inline NThreading::TFuture<NYdb::TStatus> MakeStatusFuture(NYdb::EStatus status) {
    return NThreading::MakeFuture<NYdb::TStatus>(NYdb::TStatus{status, NYdb::NIssue::TIssues{}});
}

class TestOperationResults final : public NYdb::TStatus {
public:
    TestOperationResults(NYdb::TStatus&& status, const std::string& data)
        : NYdb::TStatus(std::move(status)),
          data_(data)
    {}

    TestOperationResults(TestOperationResults&&) = default;

    const std::string& GetData() const { return data_; }

private:
    std::string data_;
};

template <typename TClient>
class RetryHandlerFixture : public ydb::ClientFixtureBase {
public:
    static constexpr std::size_t kBudgetCapacity = 6;

    template <typename Func>
    auto MakeHandler(Func func) {
        constexpr std::size_t kMaxRetries = 5;
        NYdb::NRetry::TRetryOperationSettings settings;
        settings.MaxRetries(kMaxRetries);
        settings.MaxTimeout(TDuration::Seconds(5));
        settings.SlowBackoffSettings(NYdb::NRetry::TBackoffSettings{}.SlotDuration(TDuration::Zero()));
        return std::make_shared<ydb::impl::RetryHandler<TClient, Func>>(GetClient(), budget, settings, std::move(func));
    }

    template <typename Func>
    auto Execute(Func func) {
        return ydb::impl::GetFutureValueUnchecked(MakeHandler(std::move(func))->Execute());
    }

    utils::RetryBudget budget{{.max_tokens = kBudgetCapacity, .token_ratio = 1}};

private:
    TClient& GetClient() {
        if constexpr (std::is_same_v<TClient, NYdb::NTable::TTableClient>) {
            return GetNativeTableClient();
        } else {
            return GetNativeQueryClient();
        }
    }
};

}  // namespace

UTEST_F(RetryOperationFixture, HandleOfInheritorsOfTStatus) {
    const std::string data = "qwerty";
    const auto res = RetryOperationSync(
        /*retries=*/0,
        [&data](NYdb::NTable::TSession) {
            return NThreading::MakeFuture<
                TestOperationResults>(TestOperationResults{NYdb::TStatus{kSuccess, NYdb::NIssue::TIssues{}}, data});
        }
    );
    ASSERT_EQ(res.GetData(), data);
};

UTEST_F(RetryOperationFixture, Success) {
    std::size_t attempts = 0;
    const auto res = RetryOperationSync(/*retries=*/3, [&attempts](NYdb::NTable::TSession) {
        attempts++;
        return MakeStatusFuture(kSuccess);
    });

    ASSERT_EQ(res.GetStatus(), NYdb::EStatus::SUCCESS);
    ASSERT_EQ(attempts, 1);
};

UTEST_F(RetryOperationFixture, NonRetry) {
    std::size_t attempts = 0;
    const auto res = RetryOperationSync(/*retries=*/3, [&attempts](NYdb::NTable::TSession) {
        attempts++;
        return MakeStatusFuture(kNonRetryableStatus);
    });
    ASSERT_EQ(res.GetStatus(), kNonRetryableStatus);
    ASSERT_EQ(attempts, 1);
};

UTEST_F(RetryOperationFixture, SuccessOnTheLastAttempt) {
    constexpr std::uint32_t kRetries = 5;
    std::size_t attempts = 0;
    const auto res = RetryOperationSync(/*retries=*/kRetries, [&attempts](NYdb::NTable::TSession) {
        attempts++;
        if (attempts < kRetries) {
            return MakeStatusFuture(kRetryableStatus);
        }
        return MakeStatusFuture(kSuccess);
    });
    ASSERT_EQ(res.GetStatus(), NYdb::EStatus::SUCCESS);
    ASSERT_EQ(attempts, kRetries);
};

UTEST_F(RetryOperationFixture, AttemptsIsRetriesPlusOne) {
    constexpr std::uint32_t kRetries = 5;
    std::size_t attempts = 0;
    const auto res = RetryOperationSync(
        /*retries=*/kRetries,
        [&attempts](NYdb::NTable::TSession) {
            attempts++;
            return MakeStatusFuture(kRetryableStatus);
        }
    );
    ASSERT_EQ(attempts, kRetries + 1);
    ASSERT_EQ(res.GetStatus(), kRetryableStatus);
};

UTEST_F(RetryOperationFixture, RetriesLimit) {
    // ydb-sdk has own maximum for retries, so we want to step over this
    constexpr std::uint32_t kRetries = 1000;
    std::size_t attempts = 0;
    const auto res = RetryOperationSync(
        /*retries=*/1000,
        [&attempts](NYdb::NTable::TSession) {
            attempts++;
            if (attempts < kRetries) {
                return MakeStatusFuture(kRetryableStatus);
            }
            // Unreachable!
            return MakeStatusFuture(kSuccess);
        }
    );
    ASSERT_EQ(res.GetStatus(), kRetryableStatus);
};

UTEST_F(RetryOperationFixture, Exception) {
    UASSERT_THROW_MSG(
        RetryOperationSync(
            /*retries=*/0,
            [](NYdb::NTable::TSession) {
                throw std::runtime_error{"error"};
                return MakeStatusFuture(kSuccess);
            }
        ),
        std::runtime_error,
        "error"
    );
};

using RetryHandlerClients = testing::Types<NYdb::NTable::TTableClient, NYdb::NQuery::TQueryClient>;
TYPED_UTEST_SUITE(RetryHandlerFixture, RetryHandlerClients);

TYPED_UTEST(RetryHandlerFixture, SessionAndQueryFailuresShareBudget) {
    constexpr auto kFirstAttemptDuration = std::chrono::milliseconds{20};
    std::size_t attempts = 0;
    std::size_t queries = 0;
    TDuration first_timeout;
    auto handler = this->MakeHandler([&](typename TypeParam::TSession) {
        ++queries;
        return MakeStatusFuture(kRetryableStatus);
    });

    try {
        auto future = handler->Execute([&](auto& client, auto operation, const auto& settings) {
            EXPECT_EQ(settings.MaxRetries_, 0);
            if (++attempts == 1) {
                first_timeout = settings.MaxTimeout_;
                engine::SleepFor(kFirstAttemptDuration);
            } else {
                EXPECT_LE(settings.MaxTimeout_, first_timeout - TDuration::MilliSeconds(kFirstAttemptDuration.count()));
            }
            if (attempts == 2) {
                return ydb::impl::RetryOperation(client, std::move(operation), settings);
            }
            return NThreading::MakeFuture(NYdb::TStatus{
                NYdb::EStatus::OVERLOADED,
                NYdb::NIssue::TIssues{NYdb::NIssue::TIssue{"session overloaded"}}
            });
        });
        ydb::impl::GetFutureValueUnchecked(std::move(future));
        FAIL() << "Expected the final session acquisition failure";
    } catch (const ydb::YdbResponseError& error) {
        EXPECT_EQ(error.GetStatus().GetStatus(), NYdb::EStatus::OVERLOADED);
        EXPECT_NE(error.GetStatus().GetIssues().ToOneLineString().find("session overloaded"), std::string::npos);
    }

    EXPECT_EQ(attempts, this->kBudgetCapacity / 2);
    EXPECT_EQ(queries, 1);
    EXPECT_FALSE(this->budget.CanRetry());
}

TYPED_UTEST(RetryHandlerFixture, EmptyBudgetAccountsInitialAttemptsAndRecovery) {
    for (std::size_t i = 0; i < this->kBudgetCapacity / 2; ++i) {
        this->budget.AccountFail();
    }
    ASSERT_FALSE(this->budget.CanRetry());

    std::size_t attempts = 0;
    const auto failure = this->Execute([&](TypeParam&) {
        ++attempts;
        return MakeStatusFuture(kRetryableStatus);
    });
    EXPECT_EQ(failure.GetStatus(), kRetryableStatus);
    EXPECT_EQ(attempts, 1);
    EXPECT_FALSE(this->budget.CanRetry());

    constexpr std::size_t kSuccessesToRecover = 2;
    for (std::size_t successes = 1; successes <= kSuccessesToRecover; ++successes) {
        SCOPED_TRACE(successes);
        attempts = 0;
        const auto result = this->Execute([&](TypeParam&) {
            ++attempts;
            return NThreading::MakeFuture(TestOperationResults{NYdb::TStatus{kSuccess, {}}, "payload"});
        });
        EXPECT_EQ(result.GetData(), "payload");
        EXPECT_EQ(attempts, 1);
        EXPECT_EQ(this->budget.CanRetry(), successes == kSuccessesToRecover);
    }
    this->budget.AccountFail();
    EXPECT_FALSE(this->budget.CanRetry());
}

USERVER_NAMESPACE_END
