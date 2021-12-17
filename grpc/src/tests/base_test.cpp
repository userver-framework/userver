#include <userver/utest/utest.hpp>

#include <utility>
#include <vector>

#include <userver/engine/async.hpp>
#include <userver/engine/single_consumer_event.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/engine/task/task_with_result.hpp>
#include <userver/utils/algo.hpp>

#include <tests/service_fixture_test.hpp>
#include "unit_test_client.usrv.pb.hpp"
#include "unit_test_service.usrv.pb.hpp"

USERVER_NAMESPACE_BEGIN

using namespace sample::ugrpc;
using namespace std::chrono_literals;

void CheckServerContext(::grpc::ServerContext& context) {
  const auto& client_metadata = context.client_metadata();
  EXPECT_EQ(utils::FindOptional(client_metadata, "req_header"), "value");
  context.AddTrailingMetadata("resp_header", "value");
}

class UnitTestService final : public UnitTestServiceBase {
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

using GrpcClientTest =
    GrpcServiceFixtureSimple<USERVER_NAMESPACE::UnitTestService>;

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
  auto client = MakeClient<UnitTestServiceClient>();
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
  auto client = MakeClient<UnitTestServiceClient>();
  GreetingRequest out;
  out.set_name("default_context");

  GreetingResponse in;
  EXPECT_NO_THROW(in = client.SayHello(out).Finish());
  EXPECT_EQ("Hello " + out.name(), in.name());
}

UTEST_F(GrpcClientTest, InputStream) {
  constexpr int kNumber = 42;

  auto client = MakeClient<UnitTestServiceClient>();
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
  auto client = MakeClient<UnitTestServiceClient>();
  StreamGreetingRequest out;
  out.set_name("userver");
  out.set_number(0);
  auto is = client.ReadMany(out, PrepareClientContext());

  StreamGreetingResponse in;
  EXPECT_FALSE(is.Read(in));
  CheckClientContext(is.GetContext());
}

UTEST_F(GrpcClientTest, OutputStream) {
  auto client = MakeClient<UnitTestServiceClient>();
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
  auto client = MakeClient<UnitTestServiceClient>();
  auto os = client.WriteMany(PrepareClientContext());

  StreamGreetingResponse in;
  EXPECT_NO_THROW(in = os.Finish());
  EXPECT_EQ(in.number(), 0);
  CheckClientContext(os.GetContext());
}

UTEST_F(GrpcClientTest, BidirectionalStream) {
  auto client = MakeClient<UnitTestServiceClient>();
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
  auto client = MakeClient<UnitTestServiceClient>();
  auto bs = client.Chat(PrepareClientContext());

  StreamGreetingResponse in;
  EXPECT_NO_THROW(bs.WritesDone());
  EXPECT_FALSE(bs.Read(in));
  CheckClientContext(bs.GetContext());
}

UTEST_F_MT(GrpcClientTest, MultiThreadedClientTest, 4) {
  auto client = MakeClient<UnitTestServiceClient>();
  engine::SingleConsumerEvent request_finished;
  std::vector<engine::TaskWithResult<void>> tasks;
  std::atomic<bool> keep_running{true};

  for (std::size_t i = 0; i < GetThreadCount(); ++i) {
    tasks.push_back(engine::AsyncNoSpan([&] {
      GreetingRequest out;
      out.set_name("userver");

      while (keep_running) {
        auto call = client.SayHello(out, PrepareClientContext());
        auto in = call.Finish();
        CheckClientContext(call.GetContext());
        EXPECT_EQ("Hello " + out.name(), in.name());
        request_finished.Send();
        engine::Yield();
      }
    }));
  }

  EXPECT_TRUE(request_finished.WaitForEventFor(kMaxTestWaitTime));

  // Make sure that multi-threaded requests work fine for some time
  engine::SleepFor(50ms);

  keep_running = false;
  for (auto& task : tasks) task.Get();
}

USERVER_NAMESPACE_END
