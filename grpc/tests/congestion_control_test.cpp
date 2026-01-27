#include <userver/utest/utest.hpp>

#include <ugrpc/server/middlewares/congestion_control/middleware.hpp>
#include <userver/ugrpc/tests/service_fixtures.hpp>
#include <userver/utils/algo.hpp>

#include <ugrpc/impl/rpc_metadata.hpp>

#include <tests/unit_test_client.usrv.pb.hpp>
#include <tests/unit_test_service.usrv.pb.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

class UnitTestService final : public sample::ugrpc::UnitTestServiceBase {
public:
    SayHelloResult SayHello(CallContext& /*context*/, sample::ugrpc::GreetingRequest&& request) override {
        sample::ugrpc::GreetingResponse response;
        response.set_name("Hello " + request.name());
        return response;
    }
};

class CongestionControlTest
    : public ugrpc::tests::ServiceWithClientFixture<UnitTestService, sample::ugrpc::UnitTestServiceClient> {
public:
    CongestionControlTest()
        : ugrpc::tests::ServiceWithClientFixture<UnitTestService, sample::ugrpc::UnitTestServiceClient>(
              ugrpc::server::ServerConfig{},
              ugrpc::server::Middlewares{MakeMiddleware()},
              ugrpc::client::Middlewares{}
          )
    {}

private:
    std::shared_ptr<ugrpc::server::middlewares::congestion_control::Middleware> MakeMiddleware() {
        std::shared_ptr<utils::TokenBucket>
            rate_limit = std::make_shared<utils::TokenBucket>(utils::TokenBucket::MakeUnbounded());
        rate_limit->SetMaxSize(0);
        return std::make_shared<ugrpc::server::middlewares::congestion_control::Middleware>(
            ugrpc::server::middlewares::congestion_control::Settings{},
            rate_limit
        );
    }
};

class CongestionControlCustomCodeTest
    : public ugrpc::tests::ServiceWithClientFixture<UnitTestService, sample::ugrpc::UnitTestServiceClient> {
public:
    CongestionControlCustomCodeTest()
        : ugrpc::tests::ServiceWithClientFixture<UnitTestService, sample::ugrpc::UnitTestServiceClient>(
              ugrpc::server::ServerConfig{},
              ugrpc::server::Middlewares{MakeMiddleware()},
              ugrpc::client::Middlewares{}
          )
    {}

private:
    std::shared_ptr<ugrpc::server::middlewares::congestion_control::Middleware> MakeMiddleware() {
        std::shared_ptr<utils::TokenBucket>
            rate_limit = std::make_shared<utils::TokenBucket>(utils::TokenBucket::MakeUnbounded());
        rate_limit->SetMaxSize(0);
        return std::make_shared<ugrpc::server::middlewares::congestion_control::Middleware>(
            ugrpc::server::middlewares::congestion_control::Settings{grpc::StatusCode::INTERNAL},
            rate_limit
        );
    }
};

}  // namespace

UTEST_F(CongestionControlTest, Basic) {
    sample::ugrpc::GreetingRequest out;
    out.set_name("userver");
    auto future = GetClient().AsyncSayHello(out);

    UEXPECT_THROW(future.Get(), ugrpc::client::ResourceExhaustedError);

    const auto& metadata = future.GetContext().GetClientContext().GetServerInitialMetadata();
    ASSERT_EQ(
        ugrpc::impl::kCongestionControlRatelimitReason,
        utils::FindOrDefault(metadata, ugrpc::impl::kXYaTaxiRatelimitReason)
    );
}

UTEST_F(CongestionControlCustomCodeTest, CustomCode) {
    sample::ugrpc::GreetingRequest out;
    out.set_name("userver");
    auto future = GetClient().AsyncSayHello(out);

    UEXPECT_THROW(future.Get(), ugrpc::client::InternalError);

    const auto& metadata = future.GetContext().GetClientContext().GetServerInitialMetadata();
    ASSERT_EQ(
        ugrpc::impl::kCongestionControlRatelimitReason,
        utils::FindOrDefault(metadata, ugrpc::impl::kXYaTaxiRatelimitReason)
    );
}

USERVER_NAMESPACE_END
