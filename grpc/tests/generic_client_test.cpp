#include <userver/ugrpc/client/generic.hpp>

#include <cstdint>

#include <ugrpc/client/middlewares/log/middleware.hpp>
#include <userver/ugrpc/byte_buffer_utils.hpp>
#include <userver/ugrpc/client/exceptions.hpp>
#include <userver/ugrpc/tests/service_fixtures.hpp>
#include <userver/utest/log_capture_fixture.hpp>
#include <userver/utest/utest.hpp>

#include <tests/unit_test_service.usrv.pb.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

constexpr std::string_view kSayHelloCallName =
    "sample.ugrpc.UnitTestService/SayHello";

class UnitTestService final : public sample::ugrpc::UnitTestServiceBase {
 public:
  void SayHello(SayHelloCall& call,
                sample::ugrpc::GreetingRequest&& request) override {
    sample::ugrpc::GreetingResponse response;
    response.set_name("Hello " + request.name());
    call.Finish(response);
  }
};

using GenericClientTest = ugrpc::tests::ServiceFixture<UnitTestService>;

sample::ugrpc::GreetingResponse PerformGenericUnaryCall(
    ugrpc::tests::ServiceBase& test_service) {
  auto& client_factory = test_service;

  /// [sample]
  const auto client = client_factory.MakeClient<ugrpc::client::GenericClient>();

  constexpr std::string_view call_name =
      "sample.ugrpc.UnitTestService/SayHello";

  sample::ugrpc::GreetingRequest request;
  request.set_name("generic");

  auto rpc = client.UnaryCall(call_name, ugrpc::SerializeToByteBuffer(request));

  auto response_bytes = rpc.Finish();
  sample::ugrpc::GreetingResponse response;
  if (!ugrpc::ParseFromByteBuffer(std::move(response_bytes), response)) {
    throw ugrpc::client::RpcError(rpc.GetCallName(),
                                  "Failed to parse response");
  }

  return response;
  /// [sample]
}

}  // namespace

UTEST_F(GenericClientTest, UnaryCall) {
  const auto response = PerformGenericUnaryCall(*this);
  EXPECT_EQ(response.name(), "Hello generic");
}

UTEST_F(GenericClientTest, MetricsRealUnsafe) {
  const auto client = MakeClient<ugrpc::client::GenericClient>();

  sample::ugrpc::GreetingRequest request;
  request.set_name("generic");

  ugrpc::client::GenericOptions options;
  options.metrics_call_name = std::nullopt;

  auto rpc =
      client.UnaryCall(kSayHelloCallName, ugrpc::SerializeToByteBuffer(request),
                       std::make_unique<grpc::ClientContext>(), options);
  EXPECT_EQ(rpc.GetCallName(), kSayHelloCallName);
  rpc.Finish();

  const auto stats = GetStatistics(
      "grpc.client.by-destination",
      {{"grpc_method", "SayHello"},
       {"grpc_service", "sample.ugrpc.UnitTestService"},
       {"grpc_destination", "sample.ugrpc.UnitTestService/SayHello"}});
  EXPECT_EQ(stats.SingleMetricOptional("status", {{"grpc_code", "OK"}}),
            utils::statistics::Rate{1})
      << testing::PrintToString(stats);
}

UTEST_F(GenericClientTest, MetricsDefaultCallNameIsFake) {
  PerformGenericUnaryCall(*this);

  const auto stats = GetStatistics("grpc.client.by-destination",
                                   {{"grpc_method", "Generic"},
                                    {"grpc_service", "Generic"},
                                    {"grpc_destination", "Generic/Generic"}});
  EXPECT_EQ(stats.SingleMetricOptional("status", {{"grpc_code", "OK"}}),
            utils::statistics::Rate{1})
      << testing::PrintToString(stats);
}

namespace {

using GenericClientLoggingTest =
    utest::LogCaptureFixture<ugrpc::tests::ServiceFixture<UnitTestService>>;

}  // namespace

UTEST_F(GenericClientLoggingTest, Logs) {
  PerformGenericUnaryCall(*this);

  const auto span_log = GetSingleLog(
      GetLogCapture().Filter("", {{{std::string_view("stopwatch_name"),
                                    std::string_view("external_grpc")}}}));
  EXPECT_EQ(span_log.GetTagOptional("stopwatch_name"),
            "external_grpc/sample.ugrpc.UnitTestService/SayHello")
      << span_log;
  EXPECT_EQ(span_log.GetTagOptional("grpc_code"), "OK") << span_log;
}

USERVER_NAMESPACE_END
