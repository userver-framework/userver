#include <userver/utest/utest.hpp>

#include <utility>

#include <userver/utils/algo.hpp>

#include <tests/service_fixture_test.hpp>
#include "unit_test_client.usrv.pb.hpp"
#include "unit_test_handler.usrv.pb.hpp"

USERVER_NAMESPACE_BEGIN

using namespace grpc_sample;

void CheckServerContext(::grpc::ServerContext& context) {
  const auto& client_metadata = context.client_metadata();
  EXPECT_EQ(utils::FindOptional(client_metadata, "req_header"), "value");
  context.AddTrailingMetadata("resp_header", "value");
}

class UnitTestServiceBaseHandler final : public UnitTestServiceHandlerBase {
 public:
  void SayHello(SayHelloCall& call, GreetingRequest&& request) override {
    if (request.name() != "default_context") {
      CheckServerContext(call.GetContext());
    }
    GreetingResponse response;
    response.set_name("Hello " + request.name());
    call.Finish(response);
  }

  void ReadMany(ReadManyCall& call, StreamGreetingRequest&& request) override {
    CheckServerContext(call.GetContext());
    StreamGreetingResponse response;
    response.set_name("Hello again " + request.name());
    for (int i = 0; i < request.number(); ++i) {
      response.set_number(i);
      call.Write(response);
    }
    call.Finish();
  }

  void WriteMany(WriteManyCall& call) override {
    CheckServerContext(call.GetContext());
    StreamGreetingRequest request;
    int count = 0;
    while (call.Read(request)) {
      ++count;
    }
    StreamGreetingResponse response;
    response.set_name("Hello");
    response.set_number(count);
    call.Finish(response);
  }

  void Chat(ChatCall& call) override {
    CheckServerContext(call.GetContext());
    StreamGreetingRequest request;
    StreamGreetingResponse response;
    int count = 0;
    while (call.Read(request)) {
      ++count;
      response.set_number(count);
      response.set_name("Hello " + request.name());
      call.Write(response);
    }
    call.Finish();
  }
};

using GrpcClientTest = GrpcServiceFixtureSimple<UnitTestServiceBaseHandler>;

std::unique_ptr<::grpc::ClientContext> PrepareClientContext() {
  auto context = std::make_unique<::grpc::ClientContext>();
  context->AddMetadata("req_header", "value");
  return context;
}

void CheckClientContext(const ::grpc::ClientContext& context) {
  const auto& metadata = context.GetServerTrailingMetadata();
  const auto iter = metadata.find("resp_header");
  ASSERT_NE(iter, metadata.end());
  EXPECT_EQ(iter->second, "value");
}

UTEST_F(GrpcClientTest, UnaryRPC) {
  UnitTestServiceClient client{GetChannel(), GetQueue()};
  GreetingRequest out;
  out.set_name("userver");
  auto call_for_move = client.SayHello(out, PrepareClientContext());
  auto call = std::move(call_for_move);  // test move operation

  GreetingResponse in;
  EXPECT_NO_THROW(in = call.Finish());
  CheckClientContext(call.GetContext());
  EXPECT_EQ("Hello " + out.name(), in.name());
}

UTEST_F(GrpcClientTest, UnaryRPCDefaultContext) {
  UnitTestServiceClient client{GetChannel(), GetQueue()};
  GreetingRequest out;
  out.set_name("default_context");

  GreetingResponse in;
  EXPECT_NO_THROW(in = client.SayHello(out).Finish());
  EXPECT_EQ("Hello " + out.name(), in.name());
}

UTEST_F(GrpcClientTest, InputStream) {
  constexpr int kNumber = 42;

  UnitTestServiceClient client{GetChannel(), GetQueue()};
  StreamGreetingRequest out;
  out.set_name("userver");
  out.set_number(kNumber);
  auto is_for_move = client.ReadMany(out, PrepareClientContext());
  auto is = std::move(is_for_move);  // test move operation

  StreamGreetingResponse in;
  for (auto i = 0; i < kNumber; ++i) {
    EXPECT_TRUE(is.Read(in));
    EXPECT_EQ(in.number(), i);
  }
  EXPECT_FALSE(is.Read(in));

  CheckClientContext(is.GetContext());
}

UTEST_F(GrpcClientTest, EmptyInputStream) {
  UnitTestServiceClient client{GetChannel(), GetQueue()};
  StreamGreetingRequest out;
  out.set_name("userver");
  out.set_number(0);
  auto is = client.ReadMany(out, PrepareClientContext());

  StreamGreetingResponse in;
  EXPECT_FALSE(is.Read(in));
  CheckClientContext(is.GetContext());
}

UTEST_F(GrpcClientTest, OutputStream) {
  UnitTestServiceClient client{GetChannel(), GetQueue()};
  auto number = 42;
  auto os_for_move = client.WriteMany(PrepareClientContext());
  auto os = std::move(os_for_move);  // test move operation

  StreamGreetingRequest out;
  out.set_name("userver");
  for (auto i = 0; i < number; ++i) {
    out.set_number(i);
    EXPECT_NO_THROW(os.Write(out));
  }

  StreamGreetingResponse in;
  EXPECT_NO_THROW(in = os.Finish());
  EXPECT_EQ(in.number(), number);
  CheckClientContext(os.GetContext());
}

UTEST_F(GrpcClientTest, EmptyOutputStream) {
  UnitTestServiceClient client{GetChannel(), GetQueue()};
  auto os = client.WriteMany(PrepareClientContext());

  StreamGreetingResponse in;
  EXPECT_NO_THROW(in = os.Finish());
  EXPECT_EQ(in.number(), 0);
  CheckClientContext(os.GetContext());
}

UTEST_F(GrpcClientTest, BidirectionalStream) {
  UnitTestServiceClient client{GetChannel(), GetQueue()};
  auto bs_for_move = client.Chat(PrepareClientContext());
  auto bs = std::move(bs_for_move);  // test move operation

  StreamGreetingRequest out;
  out.set_name("userver");
  StreamGreetingResponse in;

  for (auto i = 0; i < 42; ++i) {
    out.set_number(i);
    EXPECT_NO_THROW(bs.Write(out));
    EXPECT_TRUE(bs.Read(in));
    EXPECT_EQ(in.number(), i + 1);
  }
  EXPECT_NO_THROW(bs.WritesDone());
  EXPECT_FALSE(bs.Read(in));
  CheckClientContext(bs.GetContext());
}

UTEST_F(GrpcClientTest, EmptyBidirectionalStream) {
  UnitTestServiceClient client{GetChannel(), GetQueue()};
  auto bs = client.Chat(PrepareClientContext());

  StreamGreetingResponse in;
  EXPECT_NO_THROW(bs.WritesDone());
  EXPECT_FALSE(bs.Read(in));
  CheckClientContext(bs.GetContext());
}

USERVER_NAMESPACE_END
