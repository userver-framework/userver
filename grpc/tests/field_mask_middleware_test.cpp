#include <memory>

#include <google/protobuf/empty.pb.h>
#include <google/protobuf/field_mask.pb.h>
#include <google/protobuf/stubs/strutil.h>
#include <google/protobuf/util/field_mask_util.h>
#include <google/protobuf/util/message_differencer.h>
#include <grpcpp/client_context.h>
#include <grpcpp/support/config.h>
#include <grpcpp/support/status.h>
#include <gtest/gtest.h>

#include <ugrpc/server/middlewares/field_mask/middleware.hpp>
#include <userver/ugrpc/client/exceptions.hpp>
#include <userver/ugrpc/field_mask.hpp>
#include <userver/ugrpc/tests/service_fixtures.hpp>
#include <userver/utest/utest.hpp>

#include <tests/messages.pb.h>
#include <tests/unit_test_client.usrv.pb.hpp>
#include <tests/unit_test_service.usrv.pb.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

class UnitTestService final : public sample::ugrpc::UnitTestServiceBase {
public:
    SayHelloResult SayHello(CallContext&, sample::ugrpc::GreetingRequest&&) override {
        sample::ugrpc::GreetingResponse response;
        response.set_greeting("Welcome!");
        response.set_name("Hello!");
        return response;
    }

    ReadManyResult ReadMany(CallContext&, sample::ugrpc::StreamGreetingRequest&&, ReadManyWriter& writer) override {
        sample::ugrpc::StreamGreetingResponse response;
        response.set_name("Hello again!");
        response.set_number(123321);
        writer.Write(response);
        return grpc::Status::OK;
    }

    WriteManyResult WriteMany(CallContext&, WriteManyReader& reader) override {
        sample::ugrpc::StreamGreetingRequest request;
        while (reader.Read(request)) {
        }

        sample::ugrpc::StreamGreetingResponse response;
        response.set_name("Hello");
        response.set_number(456654);
        return response;
    }

    ChatResult Chat(CallContext&, ChatReaderWriter& stream) override {
        sample::ugrpc::StreamGreetingRequest request;
        while (stream.Read(request)) {
            sample::ugrpc::StreamGreetingResponse response;
            response.set_name("Hello again!");
            response.set_number(987789);
            stream.Write(response);
        }
        return grpc::Status::OK;
    }
};

class FieldMaskTest : public ugrpc::tests::ServiceFixtureBase {
protected:
    FieldMaskTest() {
        auto field_mask_middleware = std::make_shared<ugrpc::server::middlewares::field_mask::Middleware>("Field-Mask");
        SetServerMiddlewares({field_mask_middleware});
        RegisterService(service_);
        StartServer();
    }

private:
    UnitTestService service_;
};

void Compare(const google::protobuf::Message& val1, const google::protobuf::Message& val2) {
    EXPECT_EQ(std::string(val1.Utf8DebugString()), std::string(val2.Utf8DebugString()));
    EXPECT_TRUE(google::protobuf::util::MessageDifferencer::Equals(val1, val2));
}

std::unique_ptr<::grpc::ClientContext> AddFieldMask(
    const google::protobuf::FieldMask& field_mask,
    std::unique_ptr<::grpc::ClientContext> context = std::make_unique<::grpc::ClientContext>()
) {
    ::grpc::string base64_encoded;
    const auto comma_separated = google::protobuf::util::FieldMaskUtil::ToString(field_mask);
    google::protobuf::WebSafeBase64Escape(comma_separated, &base64_encoded);
    context->AddMetadata("field-mask", base64_encoded);
    return context;
}

}  // namespace

UTEST_F(FieldMaskTest, NoFieldMask) {
    const auto client = MakeClient<sample::ugrpc::UnitTestServiceClient>();
    auto response = client.SayHello({}).Finish();

    sample::ugrpc::GreetingResponse expected_response;
    expected_response.set_greeting("Welcome!");
    expected_response.set_name("Hello!");
    Compare(response, expected_response);
}

UTEST_F(FieldMaskTest, EmptyFieldMask) {
    const auto client = MakeClient<sample::ugrpc::UnitTestServiceClient>();
    auto response = client.SayHello({}, AddFieldMask({})).Finish();

    sample::ugrpc::GreetingResponse expected_response;
    expected_response.set_greeting("Welcome!");
    expected_response.set_name("Hello!");
    Compare(response, expected_response);
}

UTEST_F(FieldMaskTest, WithFieldMask) {
    google::protobuf::FieldMask field_mask;
    field_mask.add_paths("greeting");
    const auto client = MakeClient<sample::ugrpc::UnitTestServiceClient>();
    auto response = client.SayHello({}, AddFieldMask(field_mask)).Finish();

    sample::ugrpc::GreetingResponse expected_response;
    expected_response.set_greeting("Welcome!");
    Compare(response, expected_response);
}

UTEST_F(FieldMaskTest, BadFieldMaskRequest) {
    google::protobuf::FieldMask field_mask;
    field_mask.add_paths("`value_field..");
    const auto client = MakeClient<sample::ugrpc::UnitTestServiceClient>();
    try {
        client.SayHello({}, AddFieldMask(field_mask)).Finish();
        FAIL();  // Should not execute. The method must throw.
    } catch (const ugrpc::client::ErrorWithStatus& error) {
        EXPECT_EQ(error.GetStatus().error_code(), ::grpc::StatusCode::INVALID_ARGUMENT);
    }
}

UTEST_F(FieldMaskTest, BadFieldMaskResponse) {
    google::protobuf::FieldMask field_mask;
    field_mask.add_paths("non_existing_field");
    const auto client = MakeClient<sample::ugrpc::UnitTestServiceClient>();
    try {
        client.SayHello({}, AddFieldMask(field_mask)).Finish();
        FAIL();  // Should not execute. The method must throw.
    } catch (const ugrpc::client::ErrorWithStatus& error) {
        EXPECT_EQ(error.GetStatus().error_code(), ::grpc::StatusCode::INVALID_ARGUMENT);
    }
}

UTEST_F(FieldMaskTest, InputStream) {
    google::protobuf::FieldMask field_mask;
    field_mask.add_paths("name");
    const auto client = MakeClient<sample::ugrpc::UnitTestServiceClient>();
    auto call = client.ReadMany({}, AddFieldMask(field_mask));
    sample::ugrpc::StreamGreetingResponse response;
    [[maybe_unused]] bool result = call.Read(response);

    sample::ugrpc::StreamGreetingResponse expected_response;
    expected_response.set_name("Hello again!");
    Compare(response, expected_response);
}

UTEST_F(FieldMaskTest, OutputStream) {
    google::protobuf::FieldMask field_mask;
    field_mask.add_paths("number");
    const auto client = MakeClient<sample::ugrpc::UnitTestServiceClient>();
    auto call = client.WriteMany(AddFieldMask(field_mask));
    [[maybe_unused]] bool result = call.Write({});
    sample::ugrpc::StreamGreetingResponse response = call.Finish();

    sample::ugrpc::StreamGreetingResponse expected_response;
    expected_response.set_number(456654);
    Compare(response, expected_response);
}

UTEST_F(FieldMaskTest, BidirectionalStream) {
    google::protobuf::FieldMask field_mask;
    field_mask.add_paths("number");
    const auto client = MakeClient<sample::ugrpc::UnitTestServiceClient>();
    auto call = client.Chat(AddFieldMask(field_mask));

    for (std::size_t i = 0; i < 3; i++) {
        [[maybe_unused]] bool result1 = call.Write({});
        sample::ugrpc::StreamGreetingResponse response;
        [[maybe_unused]] bool result2 = call.Read(response);
        sample::ugrpc::StreamGreetingResponse expected_response;
        expected_response.set_number(987789);
        Compare(response, expected_response);
        if (i == 2) return;
    }
    FAIL();
}

USERVER_NAMESPACE_END
