#include <userver/utest/utest.hpp>

#include <string>
#include <utility>

#include <gmock/gmock.h>

#include <userver/utils/algo.hpp>

#include <tests/unit_test_client.usrv.pb.hpp>
#include <tests/unit_test_service.usrv.pb.hpp>
#include <userver/ugrpc/tests/service_fixtures.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

class SimpleMetadataService final : public sample::ugrpc::UnitTestServiceBase {
public:
    SayHelloResult SayHello(CallContext& context, sample::ugrpc::GreetingRequest&& request) override {
        /// [server_read_client_metadata]
        const std::multimap<grpc::string_ref, grpc::string_ref>& client_metadata =
            context.GetServerContext().client_metadata();
        const std::optional<grpc::string_ref> custom_header = utils::FindOptional(client_metadata, "custom-header");
        /// [server_read_client_metadata]

        /// [server_write_initial_metadata]
        context.GetServerContext().AddInitialMetadata("response-header", "response-value");
        /// [server_write_initial_metadata]

        /// [server_write_trailing_metadata]
        context.GetServerContext().AddTrailingMetadata("request-id", "req-123");
        /// [server_write_trailing_metadata]

        const std::optional<grpc::string_ref> user_id = utils::FindOptional(client_metadata, "user-id");
        context.GetServerContext().AddInitialMetadata("server-version", "1.0.0");
        context.GetServerContext().AddTrailingMetadata("processing-time", "42ms");

        std::string greeting = "Hello " + request.name();
        if (custom_header) {
            greeting += " (header: " + std::string(custom_header->data(), custom_header->size()) + ")";
        }
        if (user_id) {
            greeting += " (user: " + std::string(user_id->data(), user_id->size()) + ")";
        }

        sample::ugrpc::GreetingResponse response;
        response.set_name(greeting);
        return response;
    }

    ReadManyResult
    ReadMany(CallContext& context, sample::ugrpc::StreamGreetingRequest&& request, ReadManyWriter& writer) override {
        context.GetServerContext().AddInitialMetadata("stream-started", "true");
        context.GetServerContext().AddInitialMetadata("total-items", std::to_string(request.number()));

        sample::ugrpc::StreamGreetingResponse response;
        response.set_name("Stream response for " + request.name());

        for (int i = 0; i < request.number(); ++i) {
            response.set_number(i);
            writer.Write(response);
        }

        context.GetServerContext().AddTrailingMetadata("stream-completed", "true");

        return grpc::Status::OK;
    }
};

}  // namespace

using MetadataTest = ugrpc::tests::ServiceFixture<SimpleMetadataService>;

UTEST_F(MetadataTest, ClientSendMetadata) {
    auto client = MakeClient<sample::ugrpc::UnitTestServiceClient>();

    sample::ugrpc::GreetingRequest request;
    request.set_name("test");

    /// [client_write_metadata]
    ugrpc::client::CallOptions call_options;
    call_options.AddMetadata("custom-header", "custom-value");
    /// [client_write_metadata]
    call_options.AddMetadata("user-id", "12345");

    auto response = client.SayHello(request, std::move(call_options));
    EXPECT_EQ(response.name(), "Hello test (header: custom-value) (user: 12345)");
}

UTEST_F(MetadataTest, ClientReadInitialMetadata) {
    auto client = MakeClient<sample::ugrpc::UnitTestServiceClient>();

    sample::ugrpc::GreetingRequest request;
    request.set_name("test");

    auto future = client.AsyncSayHello(request, {});
    auto response = future.Get();

    /// [client_read_initial_metadata]
    const std::multimap<grpc::string_ref, grpc::string_ref>& initial_metadata =
        future.GetContext().GetClientContext().GetServerInitialMetadata();
    /// [client_read_initial_metadata]

    EXPECT_THAT(initial_metadata, testing::Contains(testing::Pair("response-header", "response-value")));
    EXPECT_THAT(initial_metadata, testing::Contains(testing::Pair("server-version", "1.0.0")));
}

UTEST_F(MetadataTest, ClientReadTrailingMetadata) {
    auto client = MakeClient<sample::ugrpc::UnitTestServiceClient>();

    sample::ugrpc::GreetingRequest request;
    request.set_name("test");

    auto future = client.AsyncSayHello(request, {});
    auto response = future.Get();

    /// [client_read_trailing_metadata]
    const std::multimap<grpc::string_ref, grpc::string_ref>& trailing_metadata =
        future.GetContext().GetClientContext().GetServerTrailingMetadata();
    /// [client_read_trailing_metadata]

    EXPECT_THAT(trailing_metadata, testing::Contains(testing::Pair("request-id", "req-123")));
    EXPECT_THAT(trailing_metadata, testing::Contains(testing::Pair("processing-time", "42ms")));
}

UTEST_F(MetadataTest, StreamingRequestResponseMetadata) {
    auto client = MakeClient<sample::ugrpc::UnitTestServiceClient>();

    sample::ugrpc::StreamGreetingRequest request;
    request.set_name("stream-test");
    request.set_number(3);

    ugrpc::client::CallOptions call_options;
    call_options.AddMetadata("stream-id", "stream-123");
    call_options.AddMetadata("custom-request-header", "request-value");

    auto stream = client.ReadMany(request, std::move(call_options));

    sample::ugrpc::StreamGreetingResponse response;
    int count = 0;
    while (stream.Read(response)) {
        EXPECT_EQ(response.name(), "Stream response for stream-test");
        EXPECT_EQ(response.number(), count);
        ++count;
    }
    EXPECT_EQ(count, 3);

    const grpc::ClientContext& client_context = stream.GetContext().GetClientContext();

    const std::multimap<grpc::string_ref, grpc::string_ref>& initial_metadata =
        client_context.GetServerInitialMetadata();
    EXPECT_THAT(initial_metadata, testing::Contains(testing::Pair("stream-started", "true")));
    EXPECT_THAT(initial_metadata, testing::Contains(testing::Pair("total-items", "3")));

    const std::multimap<grpc::string_ref, grpc::string_ref>& trailing_metadata =
        client_context.GetServerTrailingMetadata();
    EXPECT_THAT(trailing_metadata, testing::Contains(testing::Pair("stream-completed", "true")));
}

USERVER_NAMESPACE_END
