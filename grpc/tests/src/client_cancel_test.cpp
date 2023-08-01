#include <userver/utest/utest.hpp>

#include <utility>
#include <vector>

#include <userver/engine/async.hpp>
#include <userver/engine/single_consumer_event.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/engine/task/task_with_result.hpp>
#include <userver/utils/algo.hpp>

#include <tests/unit_test_client.usrv.pb.hpp>
#include <tests/unit_test_service.usrv.pb.hpp>
#include <userver/ugrpc/tests/service_fixtures.hpp>

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define EXPECT_CANCELLED(cmd)                                                 \
  try {                                                                       \
    cmd;                                                                      \
    FAIL() << "Expected cancellation exception, but no exception was thrown"; \
  } catch (const ugrpc::client::RpcCancelledError& e) {                       \
  } catch (const ugrpc::client::CancelledError& e) {                          \
  }

using namespace std::chrono_literals;

USERVER_NAMESPACE_BEGIN

namespace {

class UnitTestService final : public sample::ugrpc::UnitTestServiceBase {
 public:
  void SayHello(SayHelloCall& call,
                sample::ugrpc::GreetingRequest&& request) override {
    sample::ugrpc::GreetingResponse response;
    response.set_name("Hello " + request.name());
    call.Finish(response);
  }

  void ReadMany(ReadManyCall& call,
                sample::ugrpc::StreamGreetingRequest&& request) override {
    sample::ugrpc::StreamGreetingResponse response;
    response.set_name("Hello again " + request.name());
    for (int i = 0; i < request.number(); ++i) {
      response.set_number(i);
      call.Write(response);
    }
    call.Finish();
  }

  void WriteMany(WriteManyCall& call) override {
    sample::ugrpc::StreamGreetingRequest request;
    int count = 0;
    while (call.Read(request)) {
      ++count;
    }
    sample::ugrpc::StreamGreetingResponse response;
    response.set_name("Hello");
    response.set_number(count);
    call.Finish(response);
  }

  void Chat(ChatCall& call) override {
    sample::ugrpc::StreamGreetingRequest request;
    sample::ugrpc::StreamGreetingResponse response;
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

std::unique_ptr<grpc::ClientContext> PrepareClientContext() {
  auto context = std::make_unique<grpc::ClientContext>();
  return context;
}

void ExpectCancelledStats(const utils::statistics::Snapshot& stats) {
  EXPECT_EQ(stats.SingleMetric("cancelled.v2").AsRate().value, 1);
  EXPECT_EQ(stats.SingleMetric("eps.v2").AsRate().value, 1);
  EXPECT_EQ(stats.SingleMetric("rps.v2").AsRate().value, 1);
  EXPECT_EQ(stats.SingleMetric("network-error.v2").AsRate().value, 0);
  EXPECT_EQ(stats.SingleMetric("abandoned-error.v2").AsRate().value, 0);
}

}  // namespace

using GrpcClientCancel = ugrpc::tests::ServiceFixture<UnitTestService>;

// TODO(TAXICOMMON-7315) fix race in the test and re-enable
UTEST_F(GrpcClientCancel, DISABLED_UnaryCall) {
  auto client = MakeClient<sample::ugrpc::UnitTestServiceClient>();
  {
    engine::current_task::GetCancellationToken().RequestCancel();

    sample::ugrpc::GreetingRequest out;
    out.set_name("userver");
    auto call = client.SayHello(out, PrepareClientContext());

    sample::ugrpc::GreetingResponse in;
    EXPECT_CANCELLED(in = call.Finish());
  }

  const auto stats = GetStatistics(
      "grpc.client.by-destination",
      {{"grpc_destination", "sample.ugrpc.UnitTestService/SayHello"}});
  ExpectCancelledStats(stats);
}

// TODO(TAXICOMMON-7315) fix race in the test and re-enable
UTEST_F(GrpcClientCancel, DISABLED_UnaryFinish) {
  auto client = MakeClient<sample::ugrpc::UnitTestServiceClient>();
  {
    sample::ugrpc::GreetingRequest out;
    out.set_name("userver");
    auto call = client.SayHello(out, PrepareClientContext());

    engine::current_task::GetCancellationToken().RequestCancel();

    sample::ugrpc::GreetingResponse in;
    EXPECT_CANCELLED(in = call.Finish());
  }

  const auto stats = GetStatistics(
      "grpc.client.by-destination",
      {{"grpc_destination", "sample.ugrpc.UnitTestService/SayHello"}});
  ExpectCancelledStats(stats);
}

// TODO(TAXICOMMON-7315) fix race in the test and re-enable
UTEST_F(GrpcClientCancel, DISABLED_InputStreamRead) {
  auto client = MakeClient<sample::ugrpc::UnitTestServiceClient>();
  {
    sample::ugrpc::StreamGreetingRequest out;
    out.set_name("userver");
    out.set_number(1);

    auto is = client.ReadMany(out, PrepareClientContext());

    engine::current_task::GetCancellationToken().RequestCancel();

    sample::ugrpc::StreamGreetingResponse in;
    EXPECT_CANCELLED([[maybe_unused]] auto ok = is.Read(in));
  }

  const auto stats = GetStatistics(
      "grpc.client.by-destination",
      {{"grpc_destination", "sample.ugrpc.UnitTestService/ReadMany"}});
  ExpectCancelledStats(stats);
}

// TODO(TAXICOMMON-7315) fix race in the test and re-enable
UTEST_F(GrpcClientCancel, DISABLED_InputStreamCall) {
  auto client = MakeClient<sample::ugrpc::UnitTestServiceClient>();
  {
    sample::ugrpc::StreamGreetingRequest out;
    out.set_name("userver");
    out.set_number(1);

    engine::current_task::GetCancellationToken().RequestCancel();

    EXPECT_CANCELLED(auto is = client.ReadMany(out, PrepareClientContext()));
  }

  const auto stats = GetStatistics(
      "grpc.client.by-destination",
      {{"grpc_destination", "sample.ugrpc.UnitTestService/ReadMany"}});
  ExpectCancelledStats(stats);
}

// TODO(TAXICOMMON-7315) fix race in the test and re-enable
UTEST_F(GrpcClientCancel, DISABLED_OutputStreamCall) {
  auto client = MakeClient<sample::ugrpc::UnitTestServiceClient>();
  {
    engine::current_task::GetCancellationToken().RequestCancel();

    EXPECT_CANCELLED(auto os = client.WriteMany(PrepareClientContext()));
  }

  const auto stats = GetStatistics(
      "grpc.client.by-destination",
      {{"grpc_destination", "sample.ugrpc.UnitTestService/WriteMany"}});
  ExpectCancelledStats(stats);
}

// TODO(TAXICOMMON-7315) fix race in the test and re-enable
UTEST_F(GrpcClientCancel, DISABLED_OutputStreamWrite) {
  auto client = MakeClient<sample::ugrpc::UnitTestServiceClient>();
  {
    auto os = client.WriteMany(PrepareClientContext());

    engine::current_task::GetCancellationToken().RequestCancel();

    sample::ugrpc::StreamGreetingRequest out;
    out.set_name("userver");
    out.set_number(1);
    EXPECT_FALSE(os.Write(out));
  }

  const auto stats = GetStatistics(
      "grpc.client.by-destination",
      {{"grpc_destination", "sample.ugrpc.UnitTestService/WriteMany"}});
  ExpectCancelledStats(stats);
}

// TODO(TAXICOMMON-7315) fix race in the test and re-enable
UTEST_F(GrpcClientCancel, DISABLED_OutputStreamFinish) {
  auto client = MakeClient<sample::ugrpc::UnitTestServiceClient>();
  {
    auto os = client.WriteMany(PrepareClientContext());

    sample::ugrpc::StreamGreetingRequest out;
    out.set_name("userver");
    out.set_number(1);
    EXPECT_TRUE(os.Write(out));

    engine::current_task::GetCancellationToken().RequestCancel();

    EXPECT_CANCELLED(os.Finish());
  }

  const auto stats = GetStatistics(
      "grpc.client.by-destination",
      {{"grpc_destination", "sample.ugrpc.UnitTestService/WriteMany"}});
  ExpectCancelledStats(stats);
}

// TODO(TAXICOMMON-7315) fix race in the test and re-enable
UTEST_F(GrpcClientCancel, DISABLED_BidirectionalStreamCall) {
  auto client = MakeClient<sample::ugrpc::UnitTestServiceClient>();
  {
    engine::current_task::GetCancellationToken().RequestCancel();

    EXPECT_CANCELLED([[maybe_unused]] auto bs =
                         client.Chat(PrepareClientContext()));
  }

  const auto stats = GetStatistics(
      "grpc.client.by-destination",
      {{"grpc_destination", "sample.ugrpc.UnitTestService/Chat"}});
  ExpectCancelledStats(stats);
}

// TODO(TAXICOMMON-7315) fix race in the test and re-enable
UTEST_F(GrpcClientCancel, DISABLED_BidirectionalStreamRead) {
  auto client = MakeClient<sample::ugrpc::UnitTestServiceClient>();
  {
    auto bs = client.Chat(PrepareClientContext());

    engine::current_task::GetCancellationToken().RequestCancel();

    sample::ugrpc::StreamGreetingResponse in;
    EXPECT_FALSE(bs.Read(in));
  }

  const auto stats = GetStatistics(
      "grpc.client.by-destination",
      {{"grpc_destination", "sample.ugrpc.UnitTestService/Chat"}});
  ExpectCancelledStats(stats);
}

// TODO(TAXICOMMON-7315) fix race in the test and re-enable
UTEST_F(GrpcClientCancel, DISABLED_BidirectionalStreamWrite) {
  auto client = MakeClient<sample::ugrpc::UnitTestServiceClient>();
  {
    auto bs = client.Chat(PrepareClientContext());

    engine::current_task::GetCancellationToken().RequestCancel();

    sample::ugrpc::StreamGreetingRequest out{};
    EXPECT_FALSE(bs.Write(out));
  }

  const auto stats = GetStatistics(
      "grpc.client.by-destination",
      {{"grpc_destination", "sample.ugrpc.UnitTestService/Chat"}});
  ExpectCancelledStats(stats);
}

// TODO(TAXICOMMON-7315) fix race in the test and re-enable
UTEST_F(GrpcClientCancel, DISABLED_BidirectionalStreamWritesDone) {
  auto client = MakeClient<sample::ugrpc::UnitTestServiceClient>();
  {
    auto bs = client.Chat(PrepareClientContext());

    engine::current_task::GetCancellationToken().RequestCancel();

    EXPECT_FALSE(bs.WritesDone());
  }

  const auto stats = GetStatistics(
      "grpc.client.by-destination",
      {{"grpc_destination", "sample.ugrpc.UnitTestService/Chat"}});
  ExpectCancelledStats(stats);
}

USERVER_NAMESPACE_END
