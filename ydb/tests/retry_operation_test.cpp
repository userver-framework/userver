#include "test_utils.hpp"

#include <ydb/impl/future.hpp>
#include <ydb/impl/retry_context.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

class RetryOperationFixture : public ydb::ClientFixtureBase {
public:
    template <typename Func>
    auto RetryOperationSync(std::size_t retries, Func func) {
        auto settings = MakeOperationSettings(retries);
        ydb::impl::RequestContext context{GetTableClient(), ydb::Query{}, settings};

        // We have to wait future here, otherwise settings and context will be
        // destroyed and we will get `stack-use-after-scope`
        return ydb::impl::GetFutureValueUnchecked(ydb::impl::RetryOperation(context, std::move(func)));
    }

private:
    ydb::OperationSettings MakeOperationSettings(std::uint32_t retries) {
        static constexpr std::chrono::milliseconds kTimeout{3000};
        return ydb::OperationSettings{
            .retries = retries,
            .operation_timeout_ms = kTimeout,
            .cancel_after_ms = kTimeout,
            .client_timeout_ms = kTimeout,
            .get_session_timeout_ms = kTimeout,
        };
    }
};

constexpr NYdb::EStatus kSuccess = NYdb::EStatus::SUCCESS;
constexpr NYdb::EStatus kRetryableStatus = NYdb::EStatus::ABORTED;
constexpr NYdb::EStatus kNonRetryableStatus = NYdb::EStatus::BAD_REQUEST;

inline NThreading::TFuture<NYdb::TStatus> MakeStatusFuture(NYdb::EStatus status) {
    return NThreading::MakeFuture<NYdb::TStatus>(NYdb::TStatus{status, NYql::TIssues()});
}

class TestOperationResults final : public NYdb::TStatus {
public:
    TestOperationResults(NYdb::TStatus&& status, const std::string& data)
        : NYdb::TStatus(std::move(status)), data_(data) {}

    TestOperationResults(const TestOperationResults&) = default;

    const std::string& GetData() const { return data_; }

private:
    std::string data_;
};

}  // namespace

UTEST_F(RetryOperationFixture, HandleOfInheritorsOfTStatus) {
    const TestOperationResults data{NYdb::TStatus{kSuccess, NYql::TIssues()}, "qwerty"};
    auto res = RetryOperationSync(
        /*retries=*/0,
        [&data](NYdb::NTable::TSession) {
            return NThreading::MakeFuture<TestOperationResults>(TestOperationResults{data});
        }
    );
    ASSERT_EQ(res.GetData(), data.GetData());
};

UTEST_F(RetryOperationFixture, Success) {
    std::size_t attempts = 0;
    auto res = RetryOperationSync(/*retries=*/3, [&attempts](NYdb::NTable::TSession) {
        attempts++;
        return MakeStatusFuture(kSuccess);
    });

    ASSERT_EQ(res.GetStatus(), NYdb::EStatus::SUCCESS);
    ASSERT_EQ(attempts, 1);
};

UTEST_F(RetryOperationFixture, NonRetry) {
    std::size_t attempts = 0;
    auto res = RetryOperationSync(/*retries=*/3, [&attempts](NYdb::NTable::TSession) {
        attempts++;
        return MakeStatusFuture(kNonRetryableStatus);
    });
    ASSERT_EQ(res.GetStatus(), kNonRetryableStatus);
    ASSERT_EQ(attempts, 1);
};

UTEST_F(RetryOperationFixture, SuccessOnTheLastAttempt) {
    constexpr std::uint32_t kRetries = 5;
    std::size_t attempts = 0;
    auto res = RetryOperationSync(/*retries=*/kRetries, [&attempts](NYdb::NTable::TSession) {
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
    auto res = RetryOperationSync(
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
    auto res = RetryOperationSync(
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

USERVER_NAMESPACE_END
