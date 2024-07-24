#include <userver/ugrpc/server/generic_service_base.hpp>

#include <userver/engine/sleep.hpp>
#include <userver/ugrpc/byte_buffer_utils.hpp>
#include <userver/ugrpc/client/exceptions.hpp>
#include <userver/ugrpc/tests/service_fixtures.hpp>
#include <userver/utest/log_capture_fixture.hpp>
#include <userver/utest/utest.hpp>
#include <userver/utils/text_light.hpp>

#include <tests/unit_test_client.usrv.pb.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

/// [sample]
constexpr std::string_view kSayHelloCallName =
    "sample.ugrpc.UnitTestService/SayHello";

class SampleGenericService final : public ugrpc::server::GenericServiceBase {
 public:
  void Handle(Call& call) override {
    EXPECT_EQ(call.GetCallName(), kSayHelloCallName);

    grpc::ByteBuffer request_bytes;
    ASSERT_TRUE(call.Read(request_bytes));

    sample::ugrpc::GreetingRequest request;
    if (!ugrpc::ParseFromByteBuffer(std::move(request_bytes), request)) {
      call.FinishWithError(grpc::Status{grpc::StatusCode::INVALID_ARGUMENT,
                                        "Failed to parse request"});
      return;
    }

    sample::ugrpc::GreetingResponse response;
    response.set_name("Hello " + request.name());

    call.WriteAndFinish(ugrpc::SerializeToByteBuffer(response));
  }
};
/// [sample]

class GenericServiceTest : public ugrpc::tests::ServiceFixtureBase {
 protected:
  GenericServiceTest() {
    RegisterService(service_);
    StartServer();
  }

  ~GenericServiceTest() override { StopServer(); }

 private:
  SampleGenericService service_{};
};

void PerformGenericUnaryCall(
    const sample::ugrpc::UnitTestServiceClient& client) {
  sample::ugrpc::GreetingRequest out;
  out.set_name("generic");

  auto call = client.SayHello(out);

  sample::ugrpc::GreetingResponse in;
  UASSERT_NO_THROW(in = call.Finish());
  EXPECT_EQ(in.name(), "Hello generic");
}

}  // namespace

UTEST_F(GenericServiceTest, UnaryCall) {
  PerformGenericUnaryCall(MakeClient<sample::ugrpc::UnitTestServiceClient>());
}

UTEST_F(GenericServiceTest, Metrics) {
  PerformGenericUnaryCall(MakeClient<sample::ugrpc::UnitTestServiceClient>());

  // Server writes metrics after Finish, after the client might have returned
  // from Finish.
  GetServer().StopServing();

  const auto stats = GetStatistics(
      "grpc.server.by-destination",
      {{"grpc_method", "SayHello"},
       {"grpc_service", "sample.ugrpc.UnitTestService"},
       {"grpc_destination", "sample.ugrpc.UnitTestService/SayHello"}});
  UEXPECT_NO_THROW(
      EXPECT_EQ(stats.SingleMetric("status", {{"grpc_code", "OK"}}),
                utils::statistics::Rate{1})
      << testing::PrintToString(stats))
      << testing::PrintToString(stats);
}

using GenericServerLoggingTest = utest::LogCaptureFixture<GenericServiceTest>;

UTEST_F(GenericServerLoggingTest, Logs) {
  PerformGenericUnaryCall(MakeClient<sample::ugrpc::UnitTestServiceClient>());

  // Server writes metrics after Finish, after the client might have returned
  // from Finish.
  GetServer().StopServing();

  const auto span_log =
      GetSingleLog(GetLogCapture().Filter([](const utest::LogRecord& record) {
        const auto span_name = record.GetTagOptional("stopwatch_name");
        return span_name && utils::text::StartsWith(*span_name, "grpc/");
      }));
  EXPECT_EQ(span_log.GetTagOptional("stopwatch_name"),
            "grpc/sample.ugrpc.UnitTestService/SayHello")
      << span_log;
  EXPECT_EQ(span_log.GetTagOptional("grpc_code"), "OK") << span_log;
}

USERVER_NAMESPACE_END
