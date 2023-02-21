#include <userver/utest/utest.hpp>

#include <userver/ugrpc/client/exceptions.hpp>

#include <tests/service_fixture_test.hpp>
#include <tests/unit_test_client.usrv.pb.hpp>
#include <tests/unit_test_service.usrv.pb.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

std::unique_ptr<grpc::ClientContext> ContextWithDeadline() {
  auto context = std::make_unique<grpc::ClientContext>();
  context->set_deadline(std::chrono::system_clock::now() +
                        utest::kMaxTestWaitTime);
  return context;
}

}  // namespace

class GrpcServerAllUnimplementedTest : public GrpcServiceFixture {
 protected:
  GrpcServerAllUnimplementedTest() { StartServer(); }

  ~GrpcServerAllUnimplementedTest() override { StopServer(); }
};

UTEST_F(GrpcServerAllUnimplementedTest, Unimplemented) {
  auto client = MakeClient<sample::ugrpc::UnitTestServiceClient>();
  sample::ugrpc::GreetingRequest out;
  out.set_name("userver");
  UEXPECT_THROW(client.SayHello(out, ContextWithDeadline()).Finish(),
                ugrpc::client::UnimplementedError);
}

class ChatOnlyService final : public sample::ugrpc::UnitTestServiceBase {
 public:
  void Chat(ChatCall& call) override { call.Finish(); }
};

using GrpcServerSomeUnimplementedTest =
    GrpcServiceFixtureSimple<ChatOnlyService>;

UTEST_F(GrpcServerSomeUnimplementedTest, Implemented) {
  auto client = MakeClient<sample::ugrpc::UnitTestServiceClient>();
  auto call = client.Chat(ContextWithDeadline());
  EXPECT_TRUE(call.WritesDone());
  sample::ugrpc::StreamGreetingResponse response;
  EXPECT_FALSE(call.Read(response));
}

UTEST_F(GrpcServerSomeUnimplementedTest, Unimplemented) {
  auto client = MakeClient<sample::ugrpc::UnitTestServiceClient>();
  sample::ugrpc::GreetingRequest out;
  out.set_name("userver");
  UEXPECT_THROW(client.SayHello(out, ContextWithDeadline()).Finish(),
                ugrpc::client::UnimplementedError);
}

USERVER_NAMESPACE_END
