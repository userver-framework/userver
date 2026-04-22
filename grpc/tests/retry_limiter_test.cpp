#include <userver/utest/utest.hpp>

#include <atomic>
#include <memory>
#include <string>
#include <unordered_map>

#include <userver/engine/deadline.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/server/request/task_inherited_data.hpp>
#include <userver/ugrpc/client/completion_status.hpp>
#include <userver/ugrpc/client/exceptions.hpp>
#include <userver/ugrpc/client/retry_limiter.hpp>
#include <userver/ugrpc/tests/service_fixtures.hpp>

#include <dynamic_config/variables/USERVER_GRPC_CLIENT_ENABLE_DEADLINE_PROPAGATION.hpp>

#include <tests/deadline_helpers.hpp>
#include <tests/unit_test_client.usrv.pb.hpp>
#include <tests/unit_test_service.usrv.pb.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

constexpr std::string_view kSayHelloMethod = "sample.ugrpc.UnitTestService/SayHello";

struct MethodStats {
    std::atomic<int> success_count{0};
    std::atomic<int> non_retryable_error_count{0};
    std::atomic<int> retryable_error_count{0};
    std::atomic<int> timeout_fail_count{0};
    std::atomic<int> timeout_dp_fail_count{0};
    std::atomic<int> network_error_count{0};
    std::atomic<int> cancelled_count{0};
    std::atomic<int> abandoned_count{0};
};

class MockRetryLimiter final : public ugrpc::client::RetryLimiter {
public:
    MockRetryLimiter(MethodStats& stats, const std::atomic<bool>& should_retry)
        : stats_(stats),
          should_retry_(should_retry)
    {}

    void AccountCompletion(const ugrpc::client::CompletionStatus& completion_status) override {
        if (completion_status.has_value()) {
            switch (completion_status.value().error_code()) {
                case grpc::StatusCode::OK:
                    ++stats_.success_count;
                    break;
                case grpc::StatusCode::DEADLINE_EXCEEDED:
                    ++stats_.timeout_fail_count;
                    break;
                default:
                    if (ugrpc::IsRetryable(completion_status.value().error_code())) {
                        ++stats_.retryable_error_count;
                    } else {
                        ++stats_.non_retryable_error_count;
                    }
                    break;
            }
        } else {
            switch (completion_status.error()) {
                case ugrpc::client::SpecialCaseCompletionType::kNetworkError:
                    ++stats_.network_error_count;
                    break;
                case ugrpc::client::SpecialCaseCompletionType::kTimeoutDeadlinePropagated:
                    ++stats_.timeout_dp_fail_count;
                    break;
                case ugrpc::client::SpecialCaseCompletionType::kCancelled:
                    ++stats_.cancelled_count;
                    break;
                case ugrpc::client::SpecialCaseCompletionType::kAbandoned:
                    ++stats_.abandoned_count;
                    break;
            }
        }
    }

    bool CanRetry() const override { return should_retry_.load(); }

private:
    MethodStats& stats_;
    const std::atomic<bool>& should_retry_;
};

class MockRetryLimiterFactory final : public ugrpc::client::RetryLimiterFactory {
public:
    std::unique_ptr<ugrpc::client::RetryLimiter> CreateRetryLimiter(ugrpc::client::RetryLimiterSettings&& settings
    ) const override {
        std::lock_guard lock(mutex_);
        return std::make_unique<MockRetryLimiter>(stats_by_method_[settings.call_name.c_str()], should_retry_);
    }

    const MethodStats& GetStats(std::string_view method_name) {
        std::lock_guard lock(mutex_);
        auto it = stats_by_method_.find(std::string{method_name});
        EXPECT_NE(it, stats_by_method_.end());
        return it->second;
    }

    void SetShouldRetry(bool value) { should_retry_.store(value); }

private:
    mutable engine::Mutex mutex_;
    mutable std::unordered_map<std::string, MethodStats> stats_by_method_;
    std::atomic<bool> should_retry_{true};
};

class SuccessService final : public sample::ugrpc::UnitTestServiceBase {
public:
    SayHelloResult SayHello(CallContext& /*context*/, sample::ugrpc::GreetingRequest&& request) override {
        sample::ugrpc::GreetingResponse response;
        response.set_name("Hello " + request.name());
        return response;
    }
};

class UnavailableService final : public sample::ugrpc::UnitTestServiceBase {
public:
    SayHelloResult SayHello(CallContext& /*context*/, sample::ugrpc::GreetingRequest&& /*request*/) override {
        return grpc::Status(grpc::StatusCode::UNAVAILABLE, "Service unavailable");
    }
};

class InvalidArgumentService final : public sample::ugrpc::UnitTestServiceBase {
public:
    SayHelloResult SayHello(CallContext& /*context*/, sample::ugrpc::GreetingRequest&& /*request*/) override {
        return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "Invalid argument");
    }
};

class TimeoutService final : public sample::ugrpc::UnitTestServiceBase {
public:
    SayHelloResult SayHello(CallContext& /*context*/, sample::ugrpc::GreetingRequest&& /*request*/) override {
        engine::InterruptibleSleepFor(tests::kLongTimeout + tests::kShortTimeout);
        sample::ugrpc::GreetingResponse response;
        response.set_name("Should not reach here");
        return response;
    }
};

class RetryableService final : public sample::ugrpc::UnitTestServiceBase {
public:
    SayHelloResult SayHello(CallContext& /*context*/, sample::ugrpc::GreetingRequest&& request) override {
        if (request_count_++ < 3) {
            return grpc::Status(grpc::StatusCode::UNAVAILABLE, "Service unavailable");
        }
        sample::ugrpc::GreetingResponse response;
        response.set_name("Hello " + request.name());
        return response;
    }

    int GetRequestCount() const { return request_count_.load(); }

private:
    std::atomic<int> request_count_{0};
};

class NonRetryableService final : public sample::ugrpc::UnitTestServiceBase {
public:
    SayHelloResult SayHello(CallContext& /*context*/, sample::ugrpc::GreetingRequest&& /*request*/) override {
        ++request_count_;
        return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "Invalid argument");
    }

    int GetRequestCount() const { return request_count_.load(); }

private:
    std::atomic<int> request_count_{0};
};

}  // namespace

template <typename ServiceType>
// NOLINTNEXTLINE(fuchsia-multiple-inheritance)
class RetryLimiterTestBase : public ugrpc::tests::ServiceBase, public ::testing::Test {
public:
    RetryLimiterTestBase() {
        ugrpc::client::ClientFactorySettings settings;
        settings.retry_limiter_factory = &throttler_factory_;
        SetServerMiddlewares(ugrpc::tests::GetDefaultServerMiddlewares());
        SetClientMiddlewares(ugrpc::tests::GetDefaultClientMiddlewares());
        RegisterService(service_);
        StartServer(std::move(settings));
    }

    ~RetryLimiterTestBase() override { StopServer(); }

    const MethodStats& GetMethodStats(std::string_view method_name) { return throttler_factory_.GetStats(method_name); }

    void SetShouldRetry(bool value) { throttler_factory_.SetShouldRetry(value); }

    ServiceType& GetService() { return service_; }

    void EnableDeadlinePropagation() {
        ExtendDynamicConfig({{::dynamic_config::USERVER_GRPC_CLIENT_ENABLE_DEADLINE_PROPAGATION, true}});
    }

private:
    ServiceType service_;
    MockRetryLimiterFactory throttler_factory_;
};

using RetryLimiterSuccessTest = RetryLimiterTestBase<SuccessService>;
using RetryLimiterUnavailableTest = RetryLimiterTestBase<UnavailableService>;
using RetryLimiterInvalidArgumentTest = RetryLimiterTestBase<InvalidArgumentService>;
using RetryLimiterTimeoutTest = RetryLimiterTestBase<TimeoutService>;
using RetryLimiterRetryTest = RetryLimiterTestBase<RetryableService>;
using RetryLimiterNonRetryableTest = RetryLimiterTestBase<NonRetryableService>;

UTEST_F(RetryLimiterSuccessTest, SuccessfulRequestAccountsSuccess) {
    auto client = MakeClient<sample::ugrpc::UnitTestServiceClient>();

    sample::ugrpc::GreetingRequest request;
    request.set_name("test");

    sample::ugrpc::GreetingResponse response;
    UEXPECT_NO_THROW(response = client.SayHello(request));
    EXPECT_EQ(response.name(), "Hello test");

    const auto& stats = GetMethodStats(kSayHelloMethod);
    EXPECT_EQ(stats.success_count.load(), 1);
    EXPECT_EQ(stats.non_retryable_error_count.load(), 0);
    EXPECT_EQ(stats.retryable_error_count.load(), 0);
    EXPECT_EQ(stats.timeout_fail_count.load(), 0);
    EXPECT_EQ(stats.timeout_dp_fail_count.load(), 0);
    EXPECT_EQ(stats.network_error_count.load(), 0);
    EXPECT_EQ(stats.cancelled_count.load(), 0);
    EXPECT_EQ(stats.abandoned_count.load(), 0);
}

UTEST_F(RetryLimiterUnavailableTest, UnavailableRequestAccountsServerFail) {
    auto client = MakeClient<sample::ugrpc::UnitTestServiceClient>();

    sample::ugrpc::GreetingRequest request;
    request.set_name("test");

    UEXPECT_THROW(client.SayHello(request), ugrpc::client::UnavailableError);

    const auto& stats = GetMethodStats(kSayHelloMethod);
    EXPECT_EQ(stats.success_count.load(), 0);
    EXPECT_EQ(stats.non_retryable_error_count.load(), 0);
    EXPECT_GE(stats.retryable_error_count.load(), 1);
    EXPECT_EQ(stats.timeout_fail_count.load(), 0);
    EXPECT_EQ(stats.timeout_dp_fail_count.load(), 0);
    EXPECT_EQ(stats.network_error_count.load(), 0);
    EXPECT_EQ(stats.cancelled_count.load(), 0);
    EXPECT_EQ(stats.abandoned_count.load(), 0);
}

UTEST_F(RetryLimiterInvalidArgumentTest, InvalidArgumentRequestAccountsClientFail) {
    auto client = MakeClient<sample::ugrpc::UnitTestServiceClient>();

    sample::ugrpc::GreetingRequest request;
    request.set_name("test");

    UEXPECT_THROW(client.SayHello(request), ugrpc::client::InvalidArgumentError);

    const auto& stats = GetMethodStats(kSayHelloMethod);
    EXPECT_EQ(stats.success_count.load(), 0);
    EXPECT_EQ(stats.non_retryable_error_count.load(), 1);
    EXPECT_EQ(stats.retryable_error_count.load(), 0);
    EXPECT_EQ(stats.timeout_fail_count.load(), 0);
    EXPECT_EQ(stats.timeout_dp_fail_count.load(), 0);
    EXPECT_EQ(stats.network_error_count.load(), 0);
    EXPECT_EQ(stats.cancelled_count.load(), 0);
    EXPECT_EQ(stats.abandoned_count.load(), 0);
}

UTEST_F(RetryLimiterTimeoutTest, TimeoutRequestAccountsTimeoutFail) {
    auto client = MakeClient<sample::ugrpc::UnitTestServiceClient>();

    sample::ugrpc::GreetingRequest request;
    request.set_name("test");

    ugrpc::client::CallOptions call_options;
    call_options.SetTimeout(tests::kShortTimeout);

    UEXPECT_THROW(client.SayHello(request, std::move(call_options)), ugrpc::client::DeadlineExceededError);

    const auto& stats = GetMethodStats(kSayHelloMethod);
    EXPECT_EQ(stats.success_count.load(), 0);
    EXPECT_EQ(stats.non_retryable_error_count.load(), 0);
    EXPECT_EQ(stats.retryable_error_count.load(), 0);
    EXPECT_EQ(stats.timeout_fail_count.load(), 1);
    EXPECT_EQ(stats.timeout_dp_fail_count.load(), 0);
    EXPECT_EQ(stats.network_error_count.load(), 0);
    EXPECT_EQ(stats.cancelled_count.load(), 0);
    EXPECT_EQ(stats.abandoned_count.load(), 0);
}

UTEST_F(RetryLimiterRetryTest, RetryAttemptsAreAccounted) {
    auto client = MakeClient<sample::ugrpc::UnitTestServiceClient>();

    sample::ugrpc::GreetingRequest request;
    request.set_name("test");

    ugrpc::client::CallOptions call_options;
    call_options.SetAttempts(4);

    sample::ugrpc::GreetingResponse response;
    UEXPECT_NO_THROW(response = client.SayHello(request, std::move(call_options)));
    EXPECT_EQ(response.name(), "Hello test");

    const auto& stats = GetMethodStats(kSayHelloMethod);
    EXPECT_EQ(stats.success_count.load(), 1);
    EXPECT_EQ(stats.non_retryable_error_count.load(), 0);
    EXPECT_EQ(stats.retryable_error_count.load(), 3);
    EXPECT_EQ(stats.timeout_fail_count.load(), 0);
    EXPECT_EQ(stats.timeout_dp_fail_count.load(), 0);
    EXPECT_EQ(stats.network_error_count.load(), 0);
    EXPECT_EQ(stats.cancelled_count.load(), 0);
    EXPECT_EQ(stats.abandoned_count.load(), 0);

    EXPECT_EQ(GetService().GetRequestCount(), 4);
}

UTEST_F(RetryLimiterNonRetryableTest, NonRetryableErrorNotRetried) {
    auto client = MakeClient<sample::ugrpc::UnitTestServiceClient>();

    sample::ugrpc::GreetingRequest request;
    request.set_name("test");

    ugrpc::client::CallOptions call_options;
    call_options.SetAttempts(4);

    UEXPECT_THROW(client.SayHello(request, std::move(call_options)), ugrpc::client::InvalidArgumentError);

    const auto& stats = GetMethodStats(kSayHelloMethod);
    EXPECT_EQ(stats.success_count.load(), 0);
    EXPECT_EQ(stats.non_retryable_error_count.load(), 1);
    EXPECT_EQ(stats.retryable_error_count.load(), 0);
    EXPECT_EQ(stats.timeout_fail_count.load(), 0);
    EXPECT_EQ(stats.timeout_dp_fail_count.load(), 0);
    EXPECT_EQ(stats.network_error_count.load(), 0);
    EXPECT_EQ(stats.cancelled_count.load(), 0);
    EXPECT_EQ(stats.abandoned_count.load(), 0);

    EXPECT_EQ(GetService().GetRequestCount(), 1);
}

UTEST_F(RetryLimiterRetryTest, ThrottlerDisablesRetries) {
    auto client = MakeClient<sample::ugrpc::UnitTestServiceClient>();

    sample::ugrpc::GreetingRequest request;
    request.set_name("test");

    SetShouldRetry(false);

    ugrpc::client::CallOptions call_options;
    call_options.SetAttempts(4);

    UEXPECT_THROW(client.SayHello(request, std::move(call_options)), ugrpc::client::UnavailableError);

    const auto& stats = GetMethodStats(kSayHelloMethod);
    EXPECT_EQ(stats.success_count.load(), 0);
    EXPECT_EQ(stats.non_retryable_error_count.load(), 0);
    EXPECT_EQ(stats.retryable_error_count.load(), 1);
    EXPECT_EQ(stats.timeout_fail_count.load(), 0);
    EXPECT_EQ(stats.timeout_dp_fail_count.load(), 0);
    EXPECT_EQ(stats.network_error_count.load(), 0);
    EXPECT_EQ(stats.cancelled_count.load(), 0);
    EXPECT_EQ(stats.abandoned_count.load(), 0);

    EXPECT_EQ(GetService().GetRequestCount(), 1);
}

UTEST_F(RetryLimiterTimeoutTest, TimeoutAccountedSeparately) {
    auto client = MakeClient<sample::ugrpc::UnitTestServiceClient>();

    sample::ugrpc::GreetingRequest request;
    request.set_name("test");

    ugrpc::client::CallOptions call_options;
    call_options.SetTimeout(tests::kShortTimeout);

    UEXPECT_THROW(client.SayHello(request, std::move(call_options)), ugrpc::client::DeadlineExceededError);

    const auto& stats = GetMethodStats(kSayHelloMethod);
    EXPECT_EQ(stats.success_count.load(), 0);
    EXPECT_EQ(stats.non_retryable_error_count.load(), 0);
    EXPECT_EQ(stats.retryable_error_count.load(), 0);
    EXPECT_EQ(stats.timeout_fail_count.load(), 1);
    EXPECT_EQ(stats.timeout_dp_fail_count.load(), 0);
    EXPECT_EQ(stats.network_error_count.load(), 0);
    EXPECT_EQ(stats.cancelled_count.load(), 0);
    EXPECT_EQ(stats.abandoned_count.load(), 0);
}

UTEST_F(RetryLimiterTimeoutTest, CancelledRequestAccountedCorrectly) {
    auto client = MakeClient<sample::ugrpc::UnitTestServiceClient>();

    sample::ugrpc::GreetingRequest request;
    request.set_name("test");

    auto task = engine::AsyncNoSpan([&client, &request] {
        ugrpc::client::CallOptions call_options;
        call_options.SetTimeout(tests::kLongTimeout);

        UEXPECT_THROW(client.SayHello(request, std::move(call_options)), ugrpc::client::RpcCancelledError);
    });

    engine::SleepFor(tests::kShortTimeout / 10);

    task.RequestCancel();
    task.Wait();

    const auto& stats = GetMethodStats(kSayHelloMethod);
    EXPECT_EQ(stats.success_count.load(), 0);
    EXPECT_EQ(stats.non_retryable_error_count.load(), 0);
    EXPECT_EQ(stats.retryable_error_count.load(), 0);
    EXPECT_EQ(stats.timeout_fail_count.load(), 0);
    EXPECT_EQ(stats.timeout_dp_fail_count.load(), 0);
    EXPECT_EQ(stats.network_error_count.load(), 0);
    EXPECT_EQ(stats.cancelled_count.load(), 1);
    EXPECT_EQ(stats.abandoned_count.load(), 0);
}

UTEST_F(RetryLimiterTimeoutTest, AbandonedRequestAccountedCorrectly) {
    auto client = MakeClient<sample::ugrpc::UnitTestServiceClient>();

    sample::ugrpc::GreetingRequest request;
    request.set_name("test");

    {
        ugrpc::client::CallOptions call_options;
        call_options.SetTimeout(tests::kLongTimeout);

        auto call = client.AsyncSayHello(request, std::move(call_options));
        engine::SleepFor(tests::kShortTimeout);
    }

    const auto& stats = GetMethodStats(kSayHelloMethod);
    EXPECT_EQ(stats.success_count.load(), 0);
    EXPECT_EQ(stats.non_retryable_error_count.load(), 0);
    EXPECT_EQ(stats.retryable_error_count.load(), 0);
    EXPECT_EQ(stats.timeout_fail_count.load(), 0);
    EXPECT_EQ(stats.timeout_dp_fail_count.load(), 0);
    EXPECT_EQ(stats.network_error_count.load(), 0);
    EXPECT_EQ(stats.cancelled_count.load(), 0);
    EXPECT_EQ(stats.abandoned_count.load(), 1);
}

UTEST_F(RetryLimiterTimeoutTest, DeadlinePropagationTimeoutAccountedSeparately) {
    EnableDeadlinePropagation();

    auto client = MakeClient<sample::ugrpc::UnitTestServiceClient>();

    sample::ugrpc::GreetingRequest request;
    request.set_name("test");

    tests::InitTaskInheritedDeadline(engine::Deadline::FromDuration(tests::kShortTimeout));

    ugrpc::client::CallOptions call_options;
    call_options.SetTimeout(tests::kLongTimeout);

    UEXPECT_THROW(client.SayHello(request, std::move(call_options)), ugrpc::client::RpcCancelledError);

    const auto& stats = GetMethodStats(kSayHelloMethod);
    EXPECT_EQ(stats.success_count.load(), 0);
    EXPECT_EQ(stats.non_retryable_error_count.load(), 0);
    EXPECT_EQ(stats.retryable_error_count.load(), 0);
    EXPECT_EQ(stats.timeout_fail_count.load(), 0);
    EXPECT_EQ(stats.timeout_dp_fail_count.load(), 1);
    EXPECT_EQ(stats.network_error_count.load(), 0);
    EXPECT_EQ(stats.cancelled_count.load(), 0);
    EXPECT_EQ(stats.abandoned_count.load(), 0);
}

USERVER_NAMESPACE_END
