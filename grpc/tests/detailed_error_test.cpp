#include <userver/utest/utest.hpp>

#include <google/protobuf/util/message_differencer.h>

#include <userver/ugrpc/client/exceptions.hpp>
#include <userver/ugrpc/rich_status.hpp>
#include <userver/ugrpc/status_utils.hpp>
#include <userver/ugrpc/tests/service_fixtures.hpp>

#include <tests/unit_test_client.usrv.pb.hpp>
#include <tests/unit_test_service.usrv.pb.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

google::rpc::Status GetExpectedGoogleStatus() {
    google::rpc::Status status;
    status.set_code(grpc::StatusCode::RESOURCE_EXHAUSTED);
    status.set_message("message");

    google::rpc::Help help;
    auto* link = help.add_links();
    link->set_description("test_url");
    link->set_url("http://help.url/auth/fts-documentation/tvm");
    status.add_details()->PackFrom(help);

    google::rpc::QuotaFailure quota_failure;
    auto* violation = quota_failure.add_violations();
    violation->set_subject("123-pipepline000");
    violation->set_description("fts quota [fts-receive] exhausted");
    status.add_details()->PackFrom(quota_failure);

    return status;
}

const google::rpc::Status kExpectedGoogleStatus = GetExpectedGoogleStatus();

class UnitTestServiceWithDetailedError final : public sample::ugrpc::UnitTestServiceBase {
public:
    SayHelloResult SayHello(CallContext& /*context*/, sample::ugrpc::GreetingRequest&& /*request*/) override {
        return ugrpc::ToGrpcStatus(kExpectedGoogleStatus);
    }
};

class UnitTestServiceWithDetailedRichError final : public sample::ugrpc::UnitTestServiceBase {
public:
    /// [rich_status_usage]
    SayHelloResult SayHello(CallContext& /*context*/, sample::ugrpc::GreetingRequest&& /*request*/) override {
        ugrpc::RichStatus rich_status{
            grpc::StatusCode::RESOURCE_EXHAUSTED,
            "message",
            ugrpc::Help{{{"test_url", "http://help.url/auth/fts-documentation/tvm"}}},
            ugrpc::QuotaFailure{{{"123-pipepline000", "fts quota [fts-receive] exhausted"}}},
        };

        return rich_status.ToGrpcStatus();
    }
    /// [rich_status_usage]
};

template <typename Client>
void RunUnaryRPCTest(Client& client) {
    sample::ugrpc::GreetingRequest out;
    out.set_name("userver");
    try {
        client.SayHello(out);
    } catch (ugrpc::client::ResourceExhaustedError& e) {
        const auto& status = e.GetStatus();
        auto gstatus = ugrpc::ToGoogleRpcStatus(status);
        EXPECT_TRUE(gstatus.has_value());
        EXPECT_TRUE(google::protobuf::util::MessageDifferencer::Equals(*gstatus, kExpectedGoogleStatus)
        ) << "Expected:\n"
          << gstatus->Utf8DebugString()  //
          << "\nActual:\n"
          << kExpectedGoogleStatus.Utf8DebugString();
        ;
    }
}

}  // namespace

using GrpcClientWithDetailedErrorTest =
    ugrpc::tests::ServiceWithClientFixture<UnitTestServiceWithDetailedError, sample::ugrpc::UnitTestServiceClient>;

UTEST_F(GrpcClientWithDetailedErrorTest, UnaryRPC) { RunUnaryRPCTest(GetClient()); }

using GrpcClientWithDetailedRichErrorTest =
    ugrpc::tests::ServiceWithClientFixture<UnitTestServiceWithDetailedRichError, sample::ugrpc::UnitTestServiceClient>;

UTEST_F(GrpcClientWithDetailedRichErrorTest, UnaryRPCCorrectMessage) { RunUnaryRPCTest(GetClient()); }

/// [try_get_rich_error_detail]
UTEST_F(GrpcClientWithDetailedRichErrorTest, TryGetErrorDetail) {
    sample::ugrpc::GreetingRequest out;
    out.set_name("userver");
    try {
        GetClient().SayHello(out);
        FAIL() << "Expected ResourceExhaustedError";
    } catch (const ugrpc::client::ResourceExhaustedError& e) {
        const auto& status = e.GetStatus();
        auto gstatus = ugrpc::ToGoogleRpcStatus(status);
        EXPECT_TRUE(gstatus.has_value());

        const auto help_detail = ugrpc::RichStatus::TryGetDetail<ugrpc::Help>(*gstatus);
        EXPECT_TRUE(help_detail.has_value());

        EXPECT_EQ(help_detail->links.size(), 1);
        EXPECT_EQ(help_detail->links[0].description, "test_url");
        EXPECT_EQ(help_detail->links[0].url, "http://help.url/auth/fts-documentation/tvm");

        const auto quota_failure_detail = ugrpc::RichStatus::TryGetDetail<ugrpc::QuotaFailure>(*gstatus);
        EXPECT_TRUE(quota_failure_detail.has_value());

        EXPECT_EQ(quota_failure_detail->violations.size(), 1);
        EXPECT_EQ(quota_failure_detail->violations[0].subject, "123-pipepline000");
        EXPECT_EQ(quota_failure_detail->violations[0].description, "fts quota [fts-receive] exhausted");
    }
}
/// [try_get_rich_error_detail]

USERVER_NAMESPACE_END
