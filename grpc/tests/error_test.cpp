#include <userver/utest/utest.hpp>

#include <google/rpc/error_details.pb.h>
#include <google/rpc/status.pb.h>

#include <userver/engine/deadline.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/server/handlers/exceptions.hpp>
#include <userver/ugrpc/client/exceptions.hpp>
#include <userver/ugrpc/status_utils.hpp>
#include <userver/ugrpc/tests/service_fixtures.hpp>

#include <tests/unit_test_client.usrv.pb.hpp>
#include <tests/unit_test_service.usrv.pb.hpp>

using namespace std::chrono_literals;

USERVER_NAMESPACE_BEGIN

namespace {

class UnitTestServiceWithError final : public sample::ugrpc::UnitTestServiceBase {
public:
    SayHelloResult SayHello(CallContext& /*context*/, sample::ugrpc::GreetingRequest&& /*request*/) override {
        return grpc::Status{grpc::StatusCode::INTERNAL, "message", "details"};
    }

    ReadManyResult ReadMany(
        CallContext& /*context*/,
        sample::ugrpc::StreamGreetingRequest&& /*request*/,
        ReadManyWriter& /*writer*/
    ) override {
        return grpc::Status{grpc::StatusCode::INTERNAL, "message", "details"};
    }

    WriteManyResult WriteMany(CallContext& /*context*/, WriteManyReader& /*reader*/) override {
        return grpc::Status{grpc::StatusCode::INTERNAL, "message", "details"};
    }

    ChatResult Chat(CallContext& /*context*/, ChatReaderWriter& /*stream*/) override {
        return grpc::Status{grpc::StatusCode::INTERNAL, "message", "details"};
    }
};

}  // namespace

using GrpcClientErrorTest =
    ugrpc::tests::ServiceWithClientFixture<UnitTestServiceWithError, sample::ugrpc::UnitTestServiceClient>;

UTEST_F(GrpcClientErrorTest, UnaryRPC) {
    sample::ugrpc::GreetingRequest out;
    out.set_name("userver");
    UEXPECT_THROW(GetClient().SayHello(out), ugrpc::client::InternalError);
}

UTEST_F(GrpcClientErrorTest, InputStream) {
    sample::ugrpc::StreamGreetingRequest out;
    out.set_name("userver");
    out.set_number(42);
    sample::ugrpc::StreamGreetingResponse in;
    auto call = GetClient().ReadMany(out);
    UEXPECT_THROW(static_cast<void>(call.Read(in)), ugrpc::client::InternalError);
}

UTEST_F(GrpcClientErrorTest, OutputStream) {
    auto call = GetClient().WriteMany();
    UEXPECT_THROW(call.Finish(), ugrpc::client::InternalError);
}

// Disabled due to https://github.com/grpc/grpc/issues/14812
UTEST_F(GrpcClientErrorTest, DISABLED_OutputStreamErrorOnWrite) {
    auto call = GetClient().WriteMany();
    sample::ugrpc::StreamGreetingRequest out{};
    out.set_name("userver");
    out.set_number(42);
    EXPECT_TRUE(call.Write(out));

    auto write_result = true;
    const auto deadline = engine::Deadline::FromDuration(utest::kMaxTestWaitTime);
    while (!deadline.IsReached()) {
        out.set_name("write_fail");
        out.set_number(0xDEAD);
        write_result = call.Write(out);
        if (!write_result) {
            break;
        }
        engine::Yield();
    }

    EXPECT_FALSE(write_result);
    UEXPECT_THROW(call.Finish(), ugrpc::client::InternalError);
}

UTEST_F(GrpcClientErrorTest, BidirectionalStream) {
    sample::ugrpc::StreamGreetingResponse in;
    auto call = GetClient().Chat();
    UEXPECT_THROW(static_cast<void>(call.Read(in)), ugrpc::client::InternalError);
}

namespace {

class ThrowCustomService final : public sample::ugrpc::UnitTestServiceBase {
public:
    ReadManyResult ReadMany(
        CallContext& /*context*/,
        sample::ugrpc::StreamGreetingRequest&& /*request*/,
        ReadManyWriter& /*writer*/
    ) override {
        throw server::handlers::Unauthorized(server::handlers::ExternalBody{"abba"});
    }
};

}  // namespace

using GrpcThrowCustomFinish =
    ugrpc::tests::ServiceWithClientFixture<ThrowCustomService, sample::ugrpc::UnitTestServiceClient>;

UTEST_F(GrpcThrowCustomFinish, InputStream) {
    sample::ugrpc::StreamGreetingRequest out;
    out.set_name("userver");
    out.set_number(1);
    auto is = GetClient().ReadMany(out);

    sample::ugrpc::StreamGreetingResponse in;
    try {
        [[maybe_unused]] const bool result1 = is.Read(in);
        FAIL();
    } catch (const ugrpc::client::UnauthenticatedError& e) {
        EXPECT_EQ(e.GetStatus().error_code(), grpc::StatusCode::UNAUTHENTICATED);
    }
}

USERVER_NAMESPACE_END
