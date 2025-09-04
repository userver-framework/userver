#include <userver/utest/utest.hpp>

#include <buf/validate/validate.pb.h>

#include <userver/engine/async.hpp>
#include <userver/ugrpc/client/exceptions.hpp>
#include <userver/ugrpc/tests/service_fixtures.hpp>

#include <types/unit_test_client.usrv.pb.hpp>
#include <types/unit_test_service.usrv.pb.hpp>

#include <grpc-protovalidate/server/middleware.hpp>
#include "utils.hpp"

USERVER_NAMESPACE_BEGIN

namespace {

class UnitTestServiceValidator final : public types::UnitTestServiceBase {
public:
    CheckConstraintsUnaryResult CheckConstraintsUnary(CallContext&, types::ConstrainedRequest&& request) override {
        types::ConstrainedResponse response;
        response.set_field(request.field());
        return response;
    }

    CheckConstraintsStreamingResult
    CheckConstraintsStreaming(CallContext&, CheckConstraintsStreamingReaderWriter& stream) override {
        types::ConstrainedRequest request;
        types::ConstrainedResponse response{};
        while (stream.Read(request)) {
            response.set_field(request.field());
            stream.Write(response);
        }
        return grpc::Status::OK;
    }

    CheckInvalidRequestConstraintsResult CheckInvalidRequestConstraints(CallContext&, types::InvalidConstraints&&)
        override {
        return google::protobuf::Empty{};
    }
};

class GrpcServerValidatorTest : public ugrpc::tests::ServiceFixtureBase,
                                public testing::WithParamInterface<grpc_protovalidate::server::Settings> {
public:
    GrpcServerValidatorTest() {
        SetServerMiddlewares({std::make_shared<grpc_protovalidate::server::Middleware>(GetParam())});
        RegisterService(service_);
        StartServer();
    }

    ~GrpcServerValidatorTest() override { StopServer(); }

private:
    UnitTestServiceValidator service_;
};

}  // namespace

INSTANTIATE_UTEST_SUITE_P(
    /*no prefix*/,
    GrpcServerValidatorTest,
    testing::Values(
        grpc_protovalidate::server::Settings{
            .per_method =
                {
                    {"types.UnitTestService/CheckConstraintsUnary", {.fail_fast = false, .send_violations = true}},
                    {"/UnknownMethod", {.fail_fast = true, .send_violations = false}},
                }},
        grpc_protovalidate::server::Settings{
            .global =
                {
                    .fail_fast = false,
                    .send_violations = true,
                },
            .per_method =
                {
                    {"types.UnitTestService/CheckConstraintsUnary", {.fail_fast = false, .send_violations = true}},
                    {"/UnknownMethod", {.fail_fast = true, .send_violations = false}},
                }},
        grpc_protovalidate::server::Settings{
            .global =
                {
                    .fail_fast = true,
                    .send_violations = true,
                },
            .per_method =
                {
                    {"types.UnitTestService/CheckConstraintsUnary", {.fail_fast = false, .send_violations = true}},
                    {"/UnknownMethod", {.fail_fast = true, .send_violations = false}},
                }}
    )
);

UTEST_P_MT(GrpcServerValidatorTest, AllValid, 2) {
    constexpr std::size_t kRequestCount = 3;
    auto client = MakeClient<types::UnitTestServiceClient>();
    auto stream = client.CheckConstraintsStreaming();

    std::vector<types::ConstrainedMessage> messages;
    std::vector<types::ConstrainedRequest> requests(kRequestCount);
    std::vector<types::ConstrainedResponse> responses;

    types::ConstrainedMessage msg;
    types::ConstrainedResponse response;

    msg.set_required_rule(1);
    messages.push_back(std::move(msg));
    messages.push_back(tests::CreateValidMessage(2));
    messages.push_back(tests::CreateValidMessage(3));

    for (std::size_t i = 0; i < kRequestCount; ++i) {
        requests[i].set_field(static_cast<int32_t>(i));
        requests[i].mutable_messages()->Add(messages.begin(), messages.end());
    }

    // check unary method

    UASSERT_NO_THROW(response = client.CheckConstraintsUnary(requests[0]));
    EXPECT_TRUE(response.has_field());
    EXPECT_EQ(response.field(), requests[0].field());

    // check streaming method

    auto write_task = engine::AsyncNoSpan([&stream, &requests] {
        for (const auto& request : requests) {
            const bool success = stream.Write(request);
            if (!success) return false;
        }

        return stream.WritesDone();
    });

    while (stream.Read(response)) {
        responses.push_back(std::move(response));
    }

    ASSERT_TRUE(write_task.Get());
    ASSERT_EQ(responses.size(), kRequestCount);

    for (std::size_t i = 0; i < kRequestCount; ++i) {
        EXPECT_TRUE(responses[i].has_field());
        EXPECT_EQ(responses[i].field(), requests[i].field());
    }
}

UTEST_P_MT(GrpcServerValidatorTest, AllInvalid, 2) {
    constexpr std::size_t kRequestCount = 3;
    const auto& streaming_settings = GetParam().Get("types.UnitTestService/CheckConstraintsStreaming");
    auto client = MakeClient<types::UnitTestServiceClient>();
    auto stream = client.CheckConstraintsStreaming();

    std::vector<types::ConstrainedRequest> requests(kRequestCount);
    const std::vector<types::ConstrainedResponse> responses;

    types::ConstrainedRequest request;
    request.set_field(1);

    types::ConstrainedRequest invalid_request;
    invalid_request.set_field(100);
    *invalid_request.add_messages() = types::ConstrainedMessage{};
    *invalid_request.add_messages() = tests::CreateInvalidMessage();

    requests[0] = request;
    requests[1] = invalid_request;
    request.set_field(3);
    requests[2] = request;

    // check unary method

    try {
        [[maybe_unused]] auto response = client.CheckConstraintsUnary(requests[1]);
        ADD_FAILURE() << "Call must fail";
    } catch (const ugrpc::client::InvalidArgumentError& err) {
        auto violations = tests::GetViolations(err);
        ASSERT_TRUE(violations);
        EXPECT_EQ(violations->violations().size(), 20);
    } catch (...) {
        ADD_FAILURE() << "'InvalidArgumentError' exception expected";
    }

    // check streaming method

    auto write_task = engine::AsyncNoSpan([&stream, &requests] {
        for (const auto& request : requests) {
            const bool success = stream.Write(request);
            if (!success) return false;
        }

        return stream.WritesDone();
    });

    types::ConstrainedResponse response;

    ASSERT_TRUE(stream.Read(response));
    EXPECT_EQ(response.field(), 1);

    try {
        [[maybe_unused]] const bool result = stream.Read(response);
        ADD_FAILURE() << "Call must fail";
    } catch (const ugrpc::client::InvalidArgumentError& err) {
        auto violations = tests::GetViolations(err);

        if (streaming_settings.send_violations) {
            ASSERT_TRUE(violations);

            if (streaming_settings.fail_fast) {
                EXPECT_EQ(violations->violations().size(), 1);
            } else {
                EXPECT_EQ(violations->violations().size(), 20);
            }
        } else {
            EXPECT_FALSE(violations);
        }
    } catch (...) {
        ADD_FAILURE() << "'InvalidArgumentError' exception expected";
    }

    write_task.Get();
}

UTEST_P(GrpcServerValidatorTest, InvalidConstraints) {
    auto client = MakeClient<types::UnitTestServiceClient>();
    types::InvalidConstraints request;
    request.set_field(1);

    try {
        [[maybe_unused]] auto response = client.CheckInvalidRequestConstraints(std::move(request));
        ADD_FAILURE() << "Call must fail";
    } catch (const ugrpc::client::InternalError&) {
        // do nothing
    } catch (...) {
        ADD_FAILURE() << "'InternalError' exception expected";
    }
}

USERVER_NAMESPACE_END
