#include <userver/utest/utest.hpp>

#include <buf/validate/validate.pb.h>

#include <userver/engine/async.hpp>
#include <userver/grpc-protovalidate/client/exceptions.hpp>
#include <userver/ugrpc/client/exceptions.hpp>
#include <userver/ugrpc/tests/service_fixtures.hpp>

#include <types/unit_test_client.usrv.pb.hpp>
#include <types/unit_test_service.usrv.pb.hpp>

#include <grpc-protovalidate/client/middleware.hpp>
#include "utils.hpp"

USERVER_NAMESPACE_BEGIN

namespace {

types::ConstrainedResponse CreateResponse(int32_t request_id) {
    types::ConstrainedResponse response;

    if (request_id != 0) {
        // happy path
        response.set_field(request_id);

        types::ConstrainedMessage msg;
        msg.set_required_rule(5);
        *response.add_messages() = std::move(msg);
        *response.add_messages() = tests::CreateValidMessage(6);
    } else {
        // error path
        response.set_field(100);
        *response.add_messages() = types::ConstrainedMessage{};
        *response.add_messages() = tests::CreateInvalidMessage();
    }

    return response;
}

class UnitTestServiceValidator final : public types::UnitTestServiceBase {
public:
    CheckConstraintsUnaryResult CheckConstraintsUnary(CallContext&, types::ConstrainedRequest&& request) override {
        return CreateResponse(request.field());
    }

    CheckConstraintsStreamingResult CheckConstraintsStreaming(
        CallContext&,
        CheckConstraintsStreamingReaderWriter& stream
    ) override {
        types::ConstrainedRequest request;
        while (stream.Read(request)) {
            stream.Write(CreateResponse(request.field()));
        }
        return grpc::Status::OK;
    }

    CheckInvalidResponseConstraintsResult CheckInvalidResponseConstraints(CallContext&, google::protobuf::Empty&&)
        override {
        types::InvalidConstraints response;
        response.set_field(1);
        return response;
    }
};

class GrpcClientValidatorTest
    : public ugrpc::tests::ServiceWithClientFixture<UnitTestServiceValidator, types::UnitTestServiceClient>,
      public testing::WithParamInterface<grpc_protovalidate::client::Settings> {
public:
    GrpcClientValidatorTest()
        : ugrpc::tests::ServiceWithClientFixture<UnitTestServiceValidator, types::UnitTestServiceClient>(
              ugrpc::server::ServerConfig{},
              ugrpc::server::Middlewares{},
              ugrpc::client::Middlewares{std::make_shared<grpc_protovalidate::client::Middleware>(GetParam())}
          )
    {}
};

}  // namespace

INSTANTIATE_UTEST_SUITE_P(
    /*no prefix*/,
    GrpcClientValidatorTest,
    testing::Values(
        grpc_protovalidate::client::Settings{
            .per_method =
                {{"types.UnitTestService/CheckConstraintsUnary", {.fail_fast = false, .validate_requests = true}},
                 {"/UnknownMethod", {.fail_fast = true}}}
        },
        grpc_protovalidate::client::Settings{
            .global = {.fail_fast = false},
            .per_method =
                {{"types.UnitTestService/CheckConstraintsUnary", {.fail_fast = false, .validate_requests = true}},
                 {"/UnknownMethod", {.fail_fast = true}}}
        }
    )
);

UTEST_P_MT(GrpcClientValidatorTest, AllValid, 2) {
    constexpr std::size_t kRequestCount = 3;

    std::vector<types::ConstrainedRequest> requests(kRequestCount);
    for (std::size_t i = 0; i < kRequestCount; ++i) {
        requests[i].set_field(static_cast<int32_t>(i + 1));
    }

    // check unary method
    types::ConstrainedResponse response;
    UEXPECT_NO_THROW(response = GetClient().CheckConstraintsUnary(requests[0]));
    EXPECT_EQ(response.field(), requests[0].field());

    // check streaming method
    auto stream = GetClient().CheckConstraintsStreaming();
    auto write_task = engine::AsyncNoSpan([&stream, &requests] {
        for (const auto& request : requests) {
            const bool success = stream.Write(request);
            if (!success) {
                return false;
            }
        }

        return stream.WritesDone();
    });

    std::vector<types::ConstrainedResponse> responses;
    while (stream.Read(response)) {
        responses.push_back(std::move(response));
    }

    ASSERT_TRUE(write_task.Get());
    ASSERT_EQ(responses.size(), kRequestCount);

    for (std::size_t i = 0; i < kRequestCount; ++i) {
        EXPECT_EQ(responses[i].field(), requests[i].field());
    }
}

UTEST_P_MT(GrpcClientValidatorTest, AllInvalid, 2) {
    constexpr std::size_t kRequestCount = 3;

    std::vector<types::ConstrainedRequest> requests(kRequestCount);
    types::ConstrainedRequest request;
    request.set_field(1);
    requests[0] = request;
    request.set_field(0);
    requests[1] = request;
    request.set_field(3);
    requests[2] = request;

    // check unary method
    try {
        [[maybe_unused]] auto response = GetClient().CheckConstraintsUnary(requests[1]);
        ADD_FAILURE() << "Call must fail";
    } catch (const grpc_protovalidate::client::ResponseError& err) {
        EXPECT_EQ(err.GetErrorInfo().violations_size(), 20);
    } catch (...) {
        ADD_FAILURE() << "'ResponseError' exception expected";
    }

    // check streaming method
    auto stream = GetClient().CheckConstraintsStreaming();
    auto write_task = engine::AsyncNoSpan([&stream, &requests] {
        for (const auto& request : requests) {
            const bool success = stream.Write(request);
            if (!success) {
                return false;
            }
        }

        return stream.WritesDone();
    });

    types::ConstrainedResponse response;

    ASSERT_TRUE(stream.Read(response));
    EXPECT_EQ(response.field(), 1);

    try {
        [[maybe_unused]] const bool result = stream.Read(response);
        ADD_FAILURE() << "Call must fail";
    } catch (const grpc_protovalidate::client::ResponseError& err) {
        const auto& streaming_settings = GetParam().Get("types.UnitTestService/CheckConstraintsStreaming");
        if (streaming_settings.fail_fast) {
            EXPECT_EQ(err.GetErrorInfo().violations_size(), 1);
        } else {
            EXPECT_EQ(err.GetErrorInfo().violations_size(), 20);
        }
    } catch (...) {
        ADD_FAILURE() << "'InvalidArgumentError' exception expected";
    }

    write_task.Get();
}

UTEST_P(GrpcClientValidatorTest, InvalidConstraints) {
    UEXPECT_THROW_MSG(
        GetClient().CheckInvalidResponseConstraints(google::protobuf::Empty{}),
        grpc_protovalidate::client::ValidatorError,
        "'types.UnitTestService/CheckInvalidResponseConstraints' failed: validator internal error"
    );
}

UTEST_P(GrpcClientValidatorTest, InvalidRequest) {
    types::ConstrainedRequest request;
    request.set_field(100);

    UEXPECT_THROW_MSG(
        GetClient().CheckConstraintsUnary(request),
        grpc_protovalidate::client::RequestError,
        "'types.UnitTestService/CheckConstraintsUnary' failed: request violates constraints (count=1)"
    );
}

USERVER_NAMESPACE_END
