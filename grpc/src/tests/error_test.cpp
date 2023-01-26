#include <userver/utest/utest.hpp>

#include <google/rpc/error_details.pb.h>
#include <google/rpc/status.pb.h>

#include <ugrpc/impl/status.hpp>
#include <userver/ugrpc/client/exceptions.hpp>

#include <tests/service_fixture_test.hpp>
#include <tests/unit_test_client.usrv.pb.hpp>
#include <tests/unit_test_service.usrv.pb.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

class UnitTestServiceWithError final
    : public sample::ugrpc::UnitTestServiceBase {
 public:
  void SayHello(SayHelloCall& call, sample::ugrpc::GreetingRequest&&) override {
    call.FinishWithError({grpc::StatusCode::INTERNAL, "message", "details"});
  }

  void ReadMany(ReadManyCall& call,
                sample::ugrpc::StreamGreetingRequest&&) override {
    call.FinishWithError({grpc::StatusCode::INTERNAL, "message", "details"});
  }

  void WriteMany(WriteManyCall& call) override {
    call.FinishWithError({grpc::StatusCode::INTERNAL, "message", "details"});
  }

  void Chat(ChatCall& call) override {
    call.FinishWithError({grpc::StatusCode::INTERNAL, "message", "details"});
  }
};

class UnitTestServiceWithDetailedError final
    : public sample::ugrpc::UnitTestServiceBase {
 public:
  static grpc::Status MakeError() {
    google::rpc::Status status_obj;

    status_obj.set_code(grpc::StatusCode::RESOURCE_EXHAUSTED);
    status_obj.set_message("message");

    google::rpc::Help help;
    auto& link = *help.add_links();
    link.set_description("test_url");
    link.set_url("http://help.url/auth/fts-documentation/tvm");
    status_obj.add_details()->PackFrom(help);

    google::rpc::QuotaFailure quota_failure;
    auto& violation = *quota_failure.add_violations();
    violation.set_subject("123-pipepline000");
    violation.set_description("fts quota [fts-receive] exhausted");
    status_obj.add_details()->PackFrom(quota_failure);

    return ugrpc::impl::ToGrpcStatus(status_obj);
  }

  void SayHello(SayHelloCall& call, sample::ugrpc::GreetingRequest&&) override {
    call.FinishWithError(MakeError());
  }
};

}  // namespace

using GrpcClientErrorTest = GrpcServiceFixtureSimple<UnitTestServiceWithError>;

UTEST_F(GrpcClientErrorTest, UnaryRPC) {
  auto client = MakeClient<sample::ugrpc::UnitTestServiceClient>();
  sample::ugrpc::GreetingRequest out;
  out.set_name("userver");
  auto call = client.SayHello(out);
  UEXPECT_THROW(call.Finish(), ugrpc::client::InternalError);
}

UTEST_F(GrpcClientErrorTest, InputStream) {
  auto client = MakeClient<sample::ugrpc::UnitTestServiceClient>();
  sample::ugrpc::StreamGreetingRequest out;
  out.set_name("userver");
  out.set_number(42);
  sample::ugrpc::StreamGreetingResponse in;
  auto call = client.ReadMany(out);
  UEXPECT_THROW(call.Finish(), ugrpc::client::InternalError);
}

UTEST_F(GrpcClientErrorTest, OutputStream) {
  auto client = MakeClient<sample::ugrpc::UnitTestServiceClient>();
  auto call = client.WriteMany();
  UEXPECT_THROW(call.Finish(), ugrpc::client::InternalError);
}

UTEST_F(GrpcClientErrorTest, BidirectionalStream) {
  auto client = MakeClient<sample::ugrpc::UnitTestServiceClient>();
  sample::ugrpc::StreamGreetingResponse in;
  auto call = client.Chat();
  UEXPECT_THROW(call.Finish(), ugrpc::client::InternalError);
}

using GrpcClientWithDetailedErrorTest =
    GrpcServiceFixtureSimple<UnitTestServiceWithDetailedError>;

UTEST_F(GrpcClientWithDetailedErrorTest, UnaryRPC) {
  constexpr std::string_view kExpectedMessage =
      R"(code: 8 message: "message" details { )"
      R"([type.googleapis.com/google.rpc.Help] { links { description: "test_url" )"
      R"(url: "http://help.url/auth/fts-documentation/tvm" } } } details { )"
      R"([type.googleapis.com/google.rpc.QuotaFailure] { violations )"
      R"({ subject: "123-pipepline000" description: )"
      R"("fts quota [fts-receive] exhausted" } } })";

  auto client = MakeClient<sample::ugrpc::UnitTestServiceClient>();
  sample::ugrpc::GreetingRequest out;
  out.set_name("userver");
  try {
    auto call = client.SayHello(out);
    call.Finish();
  } catch (ugrpc::client::ResourceExhaustedError& e) {
    EXPECT_EQ(*(e.GetGStatusString()), kExpectedMessage);
  }
}

USERVER_NAMESPACE_END
