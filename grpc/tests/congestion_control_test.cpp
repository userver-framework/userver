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

class CongestionControlTest : public ugrpc::tests::ServiceFixtureBase {
protected:
    CongestionControlTest() {
        std::shared_ptr<utils::TokenBucket> rate_limit =
            std::make_shared<utils::TokenBucket>(utils::TokenBucket::MakeUnbounded());
        rate_limit->SetMaxSize(0);
        auto congestion_control_middleware =
            std::make_shared<ugrpc::server::middlewares::congestion_control::Middleware>(
                ugrpc::server::middlewares::congestion_control::Settings{}, rate_limit
            );
        SetServerMiddlewares({congestion_control_middleware});

        RegisterService(service_);
        StartServer();
    }

private:
    UnitTestService service_;
};

class CongestionControlCustomCodeTest : public ugrpc::tests::ServiceFixtureBase {
protected:
    CongestionControlCustomCodeTest() {
        std::shared_ptr<utils::TokenBucket> rate_limit =
            std::make_shared<utils::TokenBucket>(utils::TokenBucket::MakeUnbounded());
        rate_limit->SetMaxSize(0);
        auto congestion_control_middleware =
            std::make_shared<ugrpc::server::middlewares::congestion_control::Middleware>(
                ugrpc::server::middlewares::congestion_control::Settings{grpc::StatusCode::INTERNAL}, rate_limit
            );
        SetServerMiddlewares({congestion_control_middleware});

        RegisterService(service_);
        StartServer();
    }

private:
    UnitTestService service_;
};

}  // namespace

UTEST_F(CongestionControlTest, Basic) {
    const auto client = MakeClient<sample::ugrpc::UnitTestServiceClient>();

    sample::ugrpc::GreetingRequest out;
    out.set_name("userver");
    auto future = client.AsyncSayHello(out);

    UEXPECT_THROW(future.Get(), ugrpc::client::ResourceExhaustedError);

    const auto& metadata = future.GetContext().GetClientContext().GetServerInitialMetadata();
    ASSERT_EQ(
        ugrpc::impl::kCongestionControlRatelimitReason,
        utils::FindOrDefault(metadata, ugrpc::impl::kXYaTaxiRatelimitReason)
    );
}

UTEST_F(CongestionControlCustomCodeTest, CustomCode) {
    const auto client = MakeClient<sample::ugrpc::UnitTestServiceClient>();

    sample::ugrpc::GreetingRequest out;
    out.set_name("userver");
    auto future = client.AsyncSayHello(out);

    UEXPECT_THROW(future.Get(), ugrpc::client::InternalError);

    const auto& metadata = future.GetContext().GetClientContext().GetServerInitialMetadata();
    ASSERT_EQ(
        ugrpc::impl::kCongestionControlRatelimitReason,
        utils::FindOrDefault(metadata, ugrpc::impl::kXYaTaxiRatelimitReason)
    );
}

USERVER_NAMESPACE_END
