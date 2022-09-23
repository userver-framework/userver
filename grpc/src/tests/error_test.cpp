#include <userver/utest/utest.hpp>

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

USERVER_NAMESPACE_END
