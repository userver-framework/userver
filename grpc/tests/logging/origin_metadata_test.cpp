#include <userver/ugrpc/server/generic_service_base.hpp>

#include <gmock/gmock-matchers.h>
#include <gmock/gmock-nice-strict.h>

#include <ugrpc/client/middlewares/origin/middleware.hpp>
#include <ugrpc/server/middlewares/log/middleware.hpp>
#include <userver/ugrpc/server/metadata_utils.hpp>
#include <userver/ugrpc/tests/service_fixtures.hpp>
#include <userver/utest/log_capture_fixture.hpp>
#include <userver/utest/utest.hpp>

#include <tests/unit_test_client.usrv.pb.hpp>
#include <tests/unit_test_service.usrv.pb.hpp>
#include <tests/unit_test_service_gmock.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

template <typename ServiceType>
class WithServerLogMiddleware : public ugrpc::tests::ServiceFixture<ServiceType> {
public:
    WithServerLogMiddleware()
        : ugrpc::tests::ServiceFixture<ServiceType>(
              {},
              {std::make_shared<ugrpc::server::middlewares::log::Middleware>([] {
                  // TODO in C++20, rewrite IILE to designated initialization.
                  ugrpc::server::middlewares::log::Settings log_settings;
                  log_settings.msg_log_level = logging::Level::kInfo;
                  return log_settings;
              }())},
              {}
          ) {}
};

constexpr std::string_view kSampleUserAgent = "test-service/0.0.42";

template <typename ServiceType>
class WithClientOriginMiddleware : public ugrpc::tests::ServiceFixture<ServiceType> {
public:
    WithClientOriginMiddleware()
        : ugrpc::tests::ServiceFixture<ServiceType>(
              {},
              {},
              {std::make_shared<ugrpc::client::middlewares::origin::Middleware>([] {
                  // TODO in C++20, rewrite IILE to designated initialization.
                  ugrpc::client::middlewares::origin::Settings origin_settings;
                  origin_settings.user_agent = kSampleUserAgent;
                  return origin_settings;
              }())}
          ) {}
};

template <typename ClientType, typename Base>
class WithClient : public Base {
public:
    WithClient() : client_(this->Base::template MakeClient<ClientType>()) {}

    ClientType& GetClient() { return client_; }

private:
    ClientType client_;
};

class OriginMetadataServerTest          //
    : public utest::LogCaptureFixture<  //
          WithClient<
              sample::ugrpc::UnitTestServiceClient,
              WithServerLogMiddleware<  //
                  testing::StrictMock<tests::UnitTestServiceGmock>>>> {};

class OriginMetadataClientTest  //
    : public WithClient<
          sample::ugrpc::UnitTestServiceClient,
          WithClientOriginMiddleware<  //
              testing::StrictMock<tests::UnitTestServiceGmock>>> {};

}  // namespace

UTEST_F(OriginMetadataClientTest, UnaryCall) {
    EXPECT_CALL(GetService(), SayHello)
        .WillOnce([](const ugrpc::server::CallContext& context, sample::ugrpc::GreetingRequest&&) {
            EXPECT_THAT(
                ugrpc::server::GetRepeatedMetadata(context, "x-origin"), testing::ElementsAre(kSampleUserAgent)
            );
            return sample::ugrpc::GreetingResponse{};
        });

    [[maybe_unused]] auto response = GetClient().SayHello({});
}

UTEST_F(OriginMetadataServerTest, UnaryCall) {
    EXPECT_CALL(GetService(), SayHello).WillOnce([](auto&&...) { return sample::ugrpc::GreetingResponse{}; });

    ugrpc::client::CallOptions options;
    options.AddMetadata("x-origin", kSampleUserAgent);
    [[maybe_unused]] auto response = GetClient().SayHello({}, std::move(options));

    // Logs may be written asynchronously after the response is written. This avoids a race when checking logs.
    GetServer().StopServing();

    const auto request_log = GetSingleLog(GetLogCapture().Filter("gRPC request"));
    EXPECT_EQ(request_log.GetTagOptional("useragent"), kSampleUserAgent);
}

USERVER_NAMESPACE_END
