#include <userver/utest/utest.hpp>

#include <grpcpp/support/status.h>

#include <userver/ugrpc/server/middlewares/base.hpp>
#include <userver/ugrpc/tests/service_fixtures.hpp>
#include <userver/utest/log_capture_fixture.hpp>
#include <userver/utils/flags.hpp>

#include <tests/unit_test_client.usrv.pb.hpp>
#include <tests/unit_test_service.usrv.pb.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

class Messenger final : public sample::ugrpc::UnitTestServiceBase {
public:
    SayHelloResult SayHello(CallContext& /*context*/, ::sample::ugrpc::GreetingRequest&& /*request*/) override {
        return sample::ugrpc::GreetingResponse{};
    }
};

enum class MiddlewareFlag { kNone = 0, kErrorInRequestHook = 1 << 0, kErrorInResponseHook = 1 << 1 };

using MiddlewareFlags = utils::Flags<MiddlewareFlag>;

class Middleware final : public ugrpc::server::MiddlewareBase {
public:
    Middleware(MiddlewareFlag settings) : settings_(settings) {}

    void PostRecvMessage(ugrpc::server::MiddlewareCallContext& context, google::protobuf::Message&) const override {
        if (settings_ == MiddlewareFlag::kErrorInRequestHook) {
            return context.SetError(::grpc::Status(::grpc::StatusCode::DATA_LOSS, "Data loss error in request hook"));
        }
    }

    void PreSendMessage(ugrpc::server::MiddlewareCallContext& context, google::protobuf::Message&) const override {
        if (settings_ == MiddlewareFlag::kErrorInResponseHook) {
            return context.SetError(
                ::grpc::Status(::grpc::StatusCode::OUT_OF_RANGE, "Out of range error in response hook")
            );
        }
    }

private:
    MiddlewareFlag settings_;
};

// NOLINTNEXTLINE(fuchsia-multiple-inheritance)
class MockMessengerServiceFixture : public ugrpc::tests::ServiceFixtureBase,
                                    public testing::WithParamInterface<MiddlewareFlags> {
protected:
    MockMessengerServiceFixture() {
        SetServerMiddlewares({std::make_shared<Middleware>(static_cast<MiddlewareFlag>(GetParam().GetValue()))});
        RegisterService(service_);
        StartServer();
    }

private:
    Messenger service_;
};

}  // namespace

UTEST_P(MockMessengerServiceFixture, MiddlewareInterruption) {
    const auto client = MakeClient<sample::ugrpc::UnitTestServiceClient>();
    try {
        client.SayHello(sample::ugrpc::GreetingRequest());
        FAIL();  // Should not execute. The method must throw.
    } catch (const ugrpc::client::ErrorWithStatus& error) {
        switch (static_cast<MiddlewareFlag>(GetParam().GetValue())) {
            case MiddlewareFlag::kErrorInRequestHook: {
                EXPECT_EQ(error.GetStatus().error_code(), ::grpc::StatusCode::DATA_LOSS);
                EXPECT_EQ(error.GetStatus().error_message(), "Data loss error in request hook");
                break;
            }
            case MiddlewareFlag::kErrorInResponseHook: {
                EXPECT_EQ(error.GetStatus().error_code(), ::grpc::StatusCode::OUT_OF_RANGE);
                EXPECT_EQ(error.GetStatus().error_message(), "Out of range error in response hook");
                break;
            }
            default: {
                FAIL();  // Should not happen
            }
        }
    }
}

INSTANTIATE_UTEST_SUITE_P(
    /*no prefix*/,
    MockMessengerServiceFixture,
    testing::Values(
        MiddlewareFlags{MiddlewareFlag::kErrorInRequestHook},
        MiddlewareFlags{MiddlewareFlag::kErrorInResponseHook}
    )
);

USERVER_NAMESPACE_END
