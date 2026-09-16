#include <chrono>
#include <string>

#include <gmock/gmock.h>

#include <ugrpc/server/middlewares/log/middleware.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/ugrpc/server/middlewares/base.hpp>
#include <userver/ugrpc/tests/service_fixtures.hpp>
#include <userver/utest/log_capture_fixture.hpp>
#include <userver/utest/utest.hpp>

#include <tests/unit_test_client.usrv.pb.hpp>
#include <tests/unit_test_service.usrv.pb.hpp>
#include <tests/unit_test_service_gmock.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

using ServiceType = testing::StrictMock<tests::UnitTestServiceGmock>;
using ClientType = sample::ugrpc::UnitTestServiceClient;

constexpr std::chrono::milliseconds kFinishDelay{20};

class FinishDelayMiddleware final : public ugrpc::server::MiddlewareBase {
public:
    void PreSendStatus(ugrpc::server::MiddlewareCallContext&, grpc::Status&) const override {
        engine::SleepFor(kFinishDelay);
    }
};

class GrpcLoggingTestClientServer : public ugrpc::tests::ServiceWithClientFixture<ServiceType, ClientType> {
public:
    GrpcLoggingTestClientServer()
        : ugrpc::tests::ServiceWithClientFixture<ServiceType, ClientType>({
              .server_middlewares = {std::make_shared<FinishDelayMiddleware>(), MakeLogMiddleware()},
          })
    {}

private:
    static std::shared_ptr<ugrpc::server::middlewares::log::Middleware> MakeLogMiddleware() {
        ugrpc::server::middlewares::log::Settings settings;
        settings.msg_log_level = logging::Level::kInfo;
        return std::make_shared<ugrpc::server::middlewares::log::Middleware>(settings);
    }
};

class GrpcLoggingTest : public utest::LogCaptureFixture<GrpcLoggingTestClientServer> {};

class GrpcLoggingDisabledTestClientServer : public ugrpc::tests::ServiceWithClientFixture<ServiceType, ClientType> {
public:
    GrpcLoggingDisabledTestClientServer()
        : ugrpc::tests::ServiceWithClientFixture<ServiceType, ClientType>({
              .server_middlewares = {MakeLogMiddleware()},
          })
    {}

private:
    static std::shared_ptr<ugrpc::server::middlewares::log::Middleware> MakeLogMiddleware() {
        ugrpc::server::middlewares::log::Settings settings;
        settings.msg_log_level = logging::Level::kInfo;
        settings.local_log_level = logging::Level::kError;
        return std::make_shared<ugrpc::server::middlewares::log::Middleware>(settings);
    }
};

class GrpcLoggingDisabledTest : public utest::LogCaptureFixture<GrpcLoggingDisabledTestClientServer> {};

}  // namespace

UTEST_F(GrpcLoggingTest, UnaryDelayIncludesFinishingHooks) {
    EXPECT_CALL(GetService(), SayHello).WillOnce([](auto&&...) { return sample::ugrpc::GreetingResponse{}; });

    [[maybe_unused]] auto response = GetClient().SayHello({});

    // Logs may be written asynchronously after the response is written. This avoids a race when checking logs.
    GetServer().StopServing();

    const auto response_log = GetSingleLog(GetLogCapture().Filter("gRPC response"));
    const auto delay = response_log.GetTagOptional("delay");
    ASSERT_TRUE(delay.has_value());
    EXPECT_THAT(*delay, testing::MatchesRegex(R"([0-9]+\.[0-9]{6})"));
    EXPECT_GE(std::stod(*delay), std::chrono::duration<double>{kFinishDelay}.count());
    EXPECT_FALSE(response_log.GetTagOptional("stopwatch_units").has_value());
}

UTEST_F(GrpcLoggingDisabledTest, UnaryResponseDoesNotProduceEmptyLog) {
    EXPECT_CALL(GetService(), SayHello).WillOnce([](auto&&...) { return sample::ugrpc::GreetingResponse{}; });

    [[maybe_unused]] auto response = GetClient().SayHello({});
    GetServer().StopServing();

    EXPECT_THAT(GetLogCapture().Filter("gRPC response"), testing::IsEmpty());
}

USERVER_NAMESPACE_END
