#include <cstddef>
#include <memory>
#include <string>
#include <utility>

#include <gmock/gmock.h>

#include <ugrpc/client/middlewares/log/middleware.hpp>
#include <ugrpc/server/middlewares/log/middleware.hpp>
#include <userver/ugrpc/client/exceptions.hpp>
#include <userver/ugrpc/client/middlewares/base.hpp>
#include <userver/ugrpc/server/middlewares/base.hpp>
#include <userver/ugrpc/tests/service_fixtures.hpp>
#include <userver/utest/log_capture_fixture.hpp>
#include <userver/utest/utest.hpp>

#include <tests/unit_test_client.usrv.pb.hpp>
#include <tests/unit_test_service.usrv.pb.hpp>
#include <tests/unit_test_service_gmock.hpp>
#include <tests/unparsable_response_service.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

using ServiceType = testing::StrictMock<tests::UnitTestServiceGmock>;
using ClientType = sample::ugrpc::UnitTestServiceClient;

const std::string kMarshalledLenTag = "grpc_message_marshalled_len";
const std::string kRequestLogText = "gRPC request";
const std::string kResponseLogText = "gRPC response";
const std::string kErrorLogText = "gRPC error";
const std::string kTypeTag = "grpc_type";
const std::string kSpecialCaseCompletionType = "special_case_completion";
const std::string kStreamResponseLogText = "gRPC response stream message";
const std::string kStreamRequestLogText = "gRPC request stream message";
constexpr std::size_t kRequestNameSize = 10;
constexpr std::size_t kResponseNameSize = 1000;
constexpr std::size_t kInflatedRequestNameSize = 100000;
constexpr std::size_t kStreamMessages = 3;
constexpr std::size_t kStreamNameSizeStep = 100;

sample::ugrpc::GreetingRequest MakeRequest() {
    sample::ugrpc::GreetingRequest request;
    request.set_name(std::string(kRequestNameSize, 'q'));
    return request;
}

sample::ugrpc::GreetingResponse MakeResponse() {
    sample::ugrpc::GreetingResponse response;
    response.set_name(std::string(kResponseNameSize, 'r'));
    return response;
}

sample::ugrpc::StreamGreetingRequest MakeStreamRequest(std::size_t message_index) {
    sample::ugrpc::StreamGreetingRequest request;
    request.set_name(std::string((message_index + 1) * kStreamNameSizeStep, 'q'));
    return request;
}

sample::ugrpc::StreamGreetingResponse MakeStreamResponse(std::size_t message_index) {
    sample::ugrpc::StreamGreetingResponse response;
    response.set_name(std::string((message_index + 1) * kStreamNameSizeStep, 's'));
    return response;
}

class InflateGreetingRequestMiddleware final : public ugrpc::server::MiddlewareBase {
public:
    void PostRecvMessage(ugrpc::server::MiddlewareCallContext&, google::protobuf::Message& message) const override {
        if (auto* request = dynamic_cast<sample::ugrpc::GreetingRequest*>(&message)) {
            request->set_name(std::string(kInflatedRequestNameSize, 'i'));
        }
    }
};

std::shared_ptr<ugrpc::server::middlewares::log::Middleware> MakeServerLogMiddleware() {
    ugrpc::server::middlewares::log::Settings settings;
    settings.msg_log_level = logging::Level::kInfo;
    return std::make_shared<ugrpc::server::middlewares::log::Middleware>(settings);
}

std::shared_ptr<ugrpc::client::middlewares::log::Middleware> MakeClientLogMiddleware() {
    ugrpc::client::middlewares::log::Settings settings;
    settings.log_level = logging::Level::kInfo;
    settings.msg_log_level = logging::Level::kInfo;
    return std::make_shared<ugrpc::client::middlewares::log::Middleware>(settings);
}

class ServerLogMessageLenFixture : public ugrpc::tests::ServiceWithClientFixture<ServiceType, ClientType> {
public:
    ServerLogMessageLenFixture()
        : ugrpc::tests::ServiceWithClientFixture<ServiceType, ClientType>({
              .server_middlewares = {std::make_shared<InflateGreetingRequestMiddleware>(), MakeServerLogMiddleware()},
          })
    {}
};

class ServerLogMessageLenTest : public utest::LogCaptureFixture<ServerLogMessageLenFixture> {};

class ClientLogMessageLenFixture : public ugrpc::tests::ServiceWithClientFixture<ServiceType, ClientType> {
public:
    ClientLogMessageLenFixture()
        : ugrpc::tests::ServiceWithClientFixture<ServiceType, ClientType>({
              .client_middlewares = {MakeClientLogMiddleware()},
          })
    {}
};

class ClientLogMessageLenTest : public utest::LogCaptureFixture<ClientLogMessageLenFixture> {};

class UnparsableResponseLogFixture
    : public ugrpc::tests::ServiceWithClientFixture<tests::UnparsableResponseGenericService, ClientType> {
public:
    UnparsableResponseLogFixture()
        : ugrpc::tests::ServiceWithClientFixture<tests::UnparsableResponseGenericService, ClientType>({
              .client_middlewares = {MakeClientLogMiddleware()},
          })
    {}
};

class UnparsableResponseLogTest : public utest::LogCaptureFixture<UnparsableResponseLogFixture> {};

}  // namespace

UTEST_F(ServerLogMessageLenTest, UnaryRequestLenIsTheSizeOnTheWire) {
    EXPECT_CALL(GetService(), SayHello).WillOnce([](auto&&...) { return MakeResponse(); });

    [[maybe_unused]] auto response = GetClient().SayHello(MakeRequest());
    GetServer().StopServing();

    const auto request_log = GetSingleLog(GetLogCapture().Filter(kRequestLogText));
    EXPECT_EQ(request_log.GetTag(kMarshalledLenTag), std::to_string(MakeRequest().ByteSizeLong()));
}

UTEST_F(ServerLogMessageLenTest, UnaryResponseLenIsTheSizeOnTheWire) {
    EXPECT_CALL(GetService(), SayHello).WillOnce([](auto&&...) { return MakeResponse(); });

    [[maybe_unused]] auto response = GetClient().SayHello(MakeRequest());
    GetServer().StopServing();

    const auto response_log = GetSingleLog(GetLogCapture().Filter(kResponseLogText));
    EXPECT_EQ(response_log.GetTag(kMarshalledLenTag), std::to_string(MakeResponse().ByteSizeLong()));
}

UTEST_F(ServerLogMessageLenTest, ServerStreamingResponseLenIsTheProtobufSize) {
    EXPECT_CALL(GetService(), ReadMany)
        .WillOnce([](ugrpc::server::CallContext&,
                     sample::ugrpc::StreamGreetingRequest&&,
                     tests::UnitTestServiceGmock::ReadManyWriter& writer) {
            for (std::size_t message = 0; message < kStreamMessages; ++message) {
                writer.Write(MakeStreamResponse(message));
            }
            return tests::UnitTestServiceGmock::ReadManyResult{grpc::Status::OK};
        });

    auto stream = GetClient().ReadMany({});
    sample::ugrpc::StreamGreetingResponse response;
    while (stream.Read(response)) {
    }
    GetServer().StopServing();

    const auto logs = GetLogCapture().Filter(kStreamResponseLogText);
    ASSERT_EQ(logs.size(), kStreamMessages);
    for (std::size_t message = 0; message < kStreamMessages; ++message) {
        EXPECT_EQ(logs[message].GetTag(kMarshalledLenTag), std::to_string(MakeStreamResponse(message).ByteSizeLong()));
    }
}

UTEST_F(ClientLogMessageLenTest, UnaryRequestAndResponseLensAreTheSizesOnTheWire) {
    EXPECT_CALL(GetService(), SayHello).WillOnce([](auto&&...) { return MakeResponse(); });

    [[maybe_unused]] auto response = GetClient().SayHello(MakeRequest());

    const auto request_log = GetSingleLog(GetLogCapture().Filter(kRequestLogText));
    EXPECT_EQ(request_log.GetTag(kMarshalledLenTag), std::to_string(MakeRequest().ByteSizeLong()));
    const auto response_log = GetSingleLog(GetLogCapture().Filter(kResponseLogText));
    EXPECT_EQ(response_log.GetTag(kMarshalledLenTag), std::to_string(MakeResponse().ByteSizeLong()));
}

UTEST_F(ClientLogMessageLenTest, StreamingRequestAndResponseLensAreLogged) {
    EXPECT_CALL(GetService(), WriteMany)
        .WillOnce([](ugrpc::server::CallContext&, tests::UnitTestServiceGmock::WriteManyReader& reader) {
            sample::ugrpc::StreamGreetingRequest request;
            while (reader.Read(request)) {
            }
            return tests::UnitTestServiceGmock::WriteManyResult{MakeStreamResponse(0)};
        });
    EXPECT_CALL(GetService(), ReadMany)
        .WillOnce([](ugrpc::server::CallContext&,
                     sample::ugrpc::StreamGreetingRequest&&,
                     tests::UnitTestServiceGmock::ReadManyWriter& writer) {
            for (std::size_t message = 0; message < kStreamMessages; ++message) {
                writer.Write(MakeStreamResponse(message));
            }
            return tests::UnitTestServiceGmock::ReadManyResult{grpc::Status::OK};
        });

    auto output = GetClient().WriteMany();
    for (std::size_t message = 0; message < kStreamMessages; ++message) {
        EXPECT_TRUE(output.Write(MakeStreamRequest(message)));
    }
    UEXPECT_NO_THROW(std::ignore = output.Finish());

    auto input = GetClient().ReadMany(MakeStreamRequest(0));
    sample::ugrpc::StreamGreetingResponse response;
    while (input.Read(response)) {
    }

    const auto request_logs = GetLogCapture().Filter(kStreamRequestLogText);
    ASSERT_EQ(request_logs.size(), kStreamMessages);
    for (std::size_t message = 0; message < kStreamMessages; ++message) {
        EXPECT_EQ(
            request_logs[message].GetTag(kMarshalledLenTag),
            std::to_string(MakeStreamRequest(message).ByteSizeLong())
        );
    }

    const auto response_logs = GetLogCapture().Filter(kStreamResponseLogText);
    ASSERT_EQ(response_logs.size(), kStreamMessages);
    for (std::size_t message = 0; message < kStreamMessages; ++message) {
        EXPECT_EQ(
            response_logs[message].GetTag(kMarshalledLenTag),
            std::to_string(MakeStreamResponse(message).ByteSizeLong())
        );
    }
}

UTEST_F(UnparsableResponseLogTest, UnparsableResponseLogsWireSize) {
    UEXPECT_THROW(GetClient().SayHello(MakeRequest()), ugrpc::client::RpcInterruptedError);

    const auto error_log = GetSingleLog(GetLogCapture().Filter(kErrorLogText));
    EXPECT_EQ(error_log.GetTag(kTypeTag), kSpecialCaseCompletionType);
    EXPECT_EQ(error_log.GetTag(kMarshalledLenTag), std::to_string(tests::kUnparsableResponseSize));
}

USERVER_NAMESPACE_END
