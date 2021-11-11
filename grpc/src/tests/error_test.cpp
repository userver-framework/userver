#include <userver/utest/utest.hpp>

#include <userver/ugrpc/client/exceptions.hpp>

#include <tests/service_fixture_test.hpp>
#include "unit_test_client.usrv.pb.hpp"
#include "unit_test_handler.usrv.pb.hpp"

USERVER_NAMESPACE_BEGIN

using namespace grpc_sample;

class UnitTestServiceErrorHandler final : public UnitTestServiceHandlerBase {
 public:
  void SayHello(SayHelloCall& call, GreetingRequest&& request) override {
    call.FinishWithError({::grpc::StatusCode::INTERNAL, "message", "details"});
  }

  void ReadMany(ReadManyCall& call, StreamGreetingRequest&& request) override {
    call.FinishWithError({::grpc::StatusCode::INTERNAL, "message", "details"});
  }

  void WriteMany(WriteManyCall& call) override {
    call.FinishWithError({::grpc::StatusCode::INTERNAL, "message", "details"});
  }

  void Chat(ChatCall& call) override {
    call.FinishWithError({::grpc::StatusCode::INTERNAL, "message", "details"});
  }
};

using GrpcClientErrorTest = GrpcServiceFixture<UnitTestServiceErrorHandler>;

UTEST_F(GrpcClientErrorTest, UnaryRPC) {
  UnitTestServiceClient client{GetChannel(), GetQueue()};
  GreetingRequest out;
  out.set_name("userver");
  EXPECT_THROW(client.SayHello(out).Finish(), ugrpc::client::InternalError);
}

UTEST_F(GrpcClientErrorTest, InputStream) {
  UnitTestServiceClient client{GetChannel(), GetQueue()};
  StreamGreetingRequest out;
  out.set_name("userver");
  out.set_number(42);
  StreamGreetingResponse in;
  auto is = client.ReadMany(out);
  EXPECT_THROW((void)is.Read(in), ugrpc::client::InternalError);
}

UTEST_F(GrpcClientErrorTest, OutputStream) {
  UnitTestServiceClient client{GetChannel(), GetQueue()};
  auto os = client.WriteMany();
  EXPECT_THROW(os.Finish(), ugrpc::client::InternalError);
}

UTEST_F(GrpcClientErrorTest, BidirectionalStream) {
  UnitTestServiceClient client{GetChannel(), GetQueue()};
  StreamGreetingResponse in;
  auto bs = client.Chat();
  EXPECT_THROW((void)bs.Read(in), ugrpc::client::InternalError);
}

USERVER_NAMESPACE_END
