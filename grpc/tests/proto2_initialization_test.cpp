#include <userver/utest/utest.hpp>

#include <string_view>

#include <gmock/gmock.h>

#include <tests/middlewares_fixture.hpp>
#include <tests/proto2_service_client.usrv.pb.hpp>
#include <tests/proto2_service_service.usrv.pb.hpp>
#include <tests/server_middleware_base_gmock.hpp>
#include <userver/ugrpc/client/exceptions.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

constexpr std::string_view kExpectedResponseError =
    "Cannot send uninitialized protobuf message 'sample.ugrpc.Proto2Response': missing required fields: greeting";

sample::ugrpc::Proto2Request MakeRequestForFinalResponse() {
    sample::ugrpc::Proto2Request request;
    request.set_name("userver");
    return request;
}

sample::ugrpc::Proto2Request MakeRequestForStreamingResponse() {
    sample::ugrpc::Proto2Request request;
    request.set_name("write");
    return request;
}

class Proto2TestService final : public sample::ugrpc::Proto2TestServiceBase {
public:
    SayHelloResult SayHello(CallContext& /*context*/, sample::ugrpc::Proto2Request&& /*request*/) override {
        return sample::ugrpc::Proto2Response{};
    }

    ReadManyResult ReadMany(CallContext& /*context*/, sample::ugrpc::Proto2Request&& request, ReadManyWriter& writer)
        override {
        if (request.name() == "write") {
            writer.Write(sample::ugrpc::Proto2Response{});
            return grpc::Status::OK;
        }

        return sample::ugrpc::Proto2Response{};
    }
};

class Proto2InitializationTest
    : public tests::MiddlewaresFixture<
          tests::server::ServerMiddlewareBaseMock,
          Proto2TestService,
          sample::ugrpc::Proto2TestServiceClient,
          /*N=*/1> {};

auto ExpectUninitializedResponseStatus() {
    return [](ugrpc::server::MiddlewareCallContext&, grpc::Status& status) {
        EXPECT_EQ(status.error_code(), grpc::StatusCode::INTERNAL);
        EXPECT_EQ(status.error_message(), kExpectedResponseError);
    };
}

}  // namespace

UTEST_F(Proto2InitializationTest, UnaryUnaryResponseIsInitialized) {
    EXPECT_CALL(Middleware(), PreSendMessage).Times(0);
    EXPECT_CALL(Middleware(), PreSendStatus).WillOnce(ExpectUninitializedResponseStatus());

    UEXPECT_THROW_MSG(
        Client().SayHello(MakeRequestForFinalResponse()),
        ugrpc::client::InternalError,
        kExpectedResponseError
    );
}

UTEST_F(Proto2InitializationTest, UnaryStreamResponseIsInitialized) {
    EXPECT_CALL(Middleware(), PreSendMessage).Times(0);
    EXPECT_CALL(Middleware(), PreSendStatus).WillOnce(ExpectUninitializedResponseStatus());

    auto stream = Client().ReadMany(MakeRequestForStreamingResponse());
    sample::ugrpc::Proto2Response response;

    UEXPECT_THROW_MSG(
        [[maybe_unused]] const auto has_response = stream.Read(response),
        ugrpc::client::InternalError,
        kExpectedResponseError
    );
}

UTEST_F(Proto2InitializationTest, UnaryStreamFinalResponseIsInitialized) {
    EXPECT_CALL(Middleware(), PreSendMessage).Times(0);
    EXPECT_CALL(Middleware(), PreSendStatus).WillOnce(ExpectUninitializedResponseStatus());

    auto stream = Client().ReadMany(MakeRequestForFinalResponse());
    sample::ugrpc::Proto2Response response;

    UEXPECT_THROW_MSG(
        [[maybe_unused]] const auto has_response = stream.Read(response),
        ugrpc::client::InternalError,
        kExpectedResponseError
    );
}

USERVER_NAMESPACE_END
