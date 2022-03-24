#include <userver/utest/utest.hpp>

#include <userver/ugrpc/client/exceptions.hpp>

#include <tests/service_fixture_test.hpp>
#include "unit_test_client.usrv.pb.hpp"
#include "unit_test_service.usrv.pb.hpp"

USERVER_NAMESPACE_BEGIN

using namespace sample::ugrpc;

namespace {

class UnitTestServiceWithError final : public UnitTestServiceBase {
 public:
  void SayHello(SayHelloCall& call, GreetingRequest&& request) override {
    call.FinishWithError({grpc::StatusCode::INTERNAL, "message", "details"});
  }

  void ReadMany(ReadManyCall& call, StreamGreetingRequest&& request) override {
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
  auto client = MakeClient<UnitTestServiceClient>();
  GreetingRequest out;
  out.set_name("userver");
  auto call = client.SayHello(out);
  UEXPECT_THROW(call.Finish(), ugrpc::client::InternalError);
}

UTEST_F(GrpcClientErrorTest, InputStream) {
  auto client = MakeClient<UnitTestServiceClient>();
  StreamGreetingRequest out;
  out.set_name("userver");
  out.set_number(42);
  StreamGreetingResponse in;
  auto call = client.ReadMany(out);
  UEXPECT_THROW(call.Finish(), ugrpc::client::InternalError);
}

UTEST_F(GrpcClientErrorTest, OutputStream) {
  auto client = MakeClient<UnitTestServiceClient>();
  auto call = client.WriteMany();
  UEXPECT_THROW(call.Finish(), ugrpc::client::InternalError);
}

UTEST_F(GrpcClientErrorTest, BidirectionalStream) {
  auto client = MakeClient<UnitTestServiceClient>();
  StreamGreetingResponse in;
  auto call = client.Chat();
  UEXPECT_THROW(call.Finish(), ugrpc::client::InternalError);
}

USERVER_NAMESPACE_END
