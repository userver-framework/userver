#include <chrono>
#include <string_view>

#include <userver/dynamic_config/storage_mock.hpp>
#include <userver/engine/deadline.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/utest/utest.hpp>

#include <userver/ugrpc/client/client_qos.hpp>
#include <userver/ugrpc/client/exceptions.hpp>

#include <tests/deadline_helpers.hpp>
#include <tests/timed_out_service.hpp>
#include <tests/unit_test_client.usrv.pb.hpp>
#include <tests/unit_test_client_qos.hpp>
#include <tests/unit_test_service.usrv.pb.hpp>
#include <userver/ugrpc/tests/service_fixtures.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

std::vector<dynamic_config::KeyValue> MakeQosConfig(
    std::chrono::milliseconds timeout) {
  ugrpc::client::Qos qos;
  qos.timeout = timeout;

  ugrpc::client::ClientQos client_qos;
  client_qos.SetDefault(qos);

  return {
      {tests::kUnitTestClientQos, client_qos},
  };
}

std::vector<dynamic_config::KeyValue> MakePerRpcQosConfig(
    std::chrono::milliseconds timeout) {
  ugrpc::client::Qos qos;
  qos.timeout = timeout;

  ugrpc::client::ClientQos client_qos;
  client_qos.Set("sample.ugrpc.UnitTestService/SayHello", qos);

  return {
      {tests::kUnitTestClientQos, client_qos},
  };
}

class UnitTestOkService final : public sample::ugrpc::UnitTestServiceBase {
 public:
  void SayHello(SayHelloCall& call,
                sample::ugrpc::GreetingRequest&& request) override {
    engine::InterruptibleSleepFor(tests::kShortTimeout);

    sample::ugrpc::GreetingResponse response;
    response.set_name("Hello " + request.name());
    call.Finish(response);
  }
};

template <typename Service>
class GrpcClientQosConfigFixture
    : public ugrpc::tests::ServiceFixture<Service> {
 public:
  ugrpc::client::ClientSettings MakeClientSettingsWithQos() {
    ugrpc::client::ClientSettings client_settings;
    client_settings.client_name = "test";
    client_settings.endpoint = this->GetEndpoint();
    client_settings.client_qos = &tests::kUnitTestClientQos;
    return client_settings;
  }
};

using GrpcClientQosConfigOk = GrpcClientQosConfigFixture<UnitTestOkService>;

class GrpcClientQosConfigExceeded
    : public GrpcClientQosConfigFixture<tests::TimedOutUnitTestService> {
 public:
  GrpcClientQosConfigExceeded()
      : client_deadline_(engine::Deadline::FromDuration(tests::kShortTimeout)),
        long_deadline_(engine::Deadline::FromDuration(tests::kLongTimeout)) {}

  ~GrpcClientQosConfigExceeded() override {
    EXPECT_TRUE(client_deadline_.IsReached());
    // Check that the waiting has been interrupted after kShortTimeout.
    EXPECT_FALSE(long_deadline_.IsReached());
  }

 private:
  engine::Deadline client_deadline_;
  engine::Deadline long_deadline_;
};

const sample::ugrpc::GreetingRequest kRequest = [] {
  sample::ugrpc::GreetingRequest request;
  request.set_name("userver");
  return request;
}();

}  // namespace

UTEST_F(GrpcClientQosConfigOk, Ok) {
  auto client =
      GetClientFactory().MakeClient<sample::ugrpc::UnitTestServiceClient>(
          MakeClientSettingsWithQos());
  ExtendDynamicConfig(MakeQosConfig(utest::kMaxTestWaitTime));

  auto call = client.SayHello(kRequest);

  sample::ugrpc::GreetingResponse response;
  UEXPECT_NO_THROW(response = call.Finish());
  EXPECT_EQ(response.name(), "Hello " + kRequest.name());
}

UTEST_F(GrpcClientQosConfigOk, ContextDeadlineOverrides) {
  auto client =
      GetClientFactory().MakeClient<sample::ugrpc::UnitTestServiceClient>(
          MakeClientSettingsWithQos());
  ExtendDynamicConfig(MakeQosConfig(std::chrono::milliseconds{1}));

  auto context = std::make_unique<grpc::ClientContext>();
  context->set_deadline(
      engine::Deadline::FromDuration(utest::kMaxTestWaitTime));
  auto call = client.SayHello(kRequest, std::move(context));

  sample::ugrpc::GreetingResponse response;
  UEXPECT_NO_THROW(response = call.Finish());
  EXPECT_EQ(response.name(), "Hello " + kRequest.name());
}

UTEST_F(GrpcClientQosConfigOk, UserQosOverridesEverything) {
  auto client =
      GetClientFactory().MakeClient<sample::ugrpc::UnitTestServiceClient>(
          MakeClientSettingsWithQos());
  ExtendDynamicConfig(MakeQosConfig(std::chrono::milliseconds{1}));

  auto context = std::make_unique<grpc::ClientContext>();
  context->set_deadline(
      engine::Deadline::FromDuration(std::chrono::milliseconds{1}));

  ugrpc::client::Qos qos;
  qos.timeout = tests::kLongTimeout;
  auto call = client.SayHello(kRequest, std::move(context), qos);

  sample::ugrpc::GreetingResponse response;
  UEXPECT_NO_THROW(response = call.Finish());
  EXPECT_EQ(response.name(), "Hello " + kRequest.name());
}

UTEST_F(GrpcClientQosConfigExceeded, DefaultTimeout) {
  auto client =
      GetClientFactory().MakeClient<sample::ugrpc::UnitTestServiceClient>(
          MakeClientSettingsWithQos());

  ExtendDynamicConfig(MakeQosConfig(tests::kShortTimeout));

  auto call = client.SayHello(kRequest);

  sample::ugrpc::GreetingResponse response;
  UEXPECT_THROW(response = call.Finish(), ugrpc::client::DeadlineExceededError);
}

UTEST_F(GrpcClientQosConfigExceeded, PerRpcTimeout) {
  auto client =
      GetClientFactory().MakeClient<sample::ugrpc::UnitTestServiceClient>(
          MakeClientSettingsWithQos());

  ExtendDynamicConfig(MakePerRpcQosConfig(tests::kShortTimeout));

  auto call = client.SayHello(kRequest);

  sample::ugrpc::GreetingResponse response;
  UEXPECT_THROW(response = call.Finish(), ugrpc::client::DeadlineExceededError);
}

UTEST_F(GrpcClientQosConfigExceeded, DeadlinePropagationWorks) {
  auto client =
      GetClientFactory().MakeClient<sample::ugrpc::UnitTestServiceClient>(
          MakeClientSettingsWithQos());

  ExtendDynamicConfig(MakeQosConfig(utest::kMaxTestWaitTime));
  tests::InitTaskInheritedDeadline(
      engine::Deadline::FromDuration(tests::kShortTimeout));

  auto call = client.SayHello(kRequest);

  sample::ugrpc::GreetingResponse response;
  UEXPECT_THROW(response = call.Finish(), ugrpc::client::DeadlineExceededError);
}

UTEST_F(GrpcClientQosConfigExceeded, ContextDeadlineOverrides) {
  auto client =
      GetClientFactory().MakeClient<sample::ugrpc::UnitTestServiceClient>(
          MakeClientSettingsWithQos());

  ExtendDynamicConfig(MakeQosConfig(utest::kMaxTestWaitTime));

  auto context = std::make_unique<grpc::ClientContext>();
  context->set_deadline(engine::Deadline::FromDuration(tests::kShortTimeout));
  auto call = client.SayHello(kRequest, std::move(context));

  sample::ugrpc::GreetingResponse response;
  UEXPECT_THROW(response = call.Finish(), ugrpc::client::DeadlineExceededError);
}

UTEST_F(GrpcClientQosConfigExceeded, UserQosOverridesEverything) {
  auto client =
      GetClientFactory().MakeClient<sample::ugrpc::UnitTestServiceClient>(
          MakeClientSettingsWithQos());

  ExtendDynamicConfig(MakeQosConfig(utest::kMaxTestWaitTime));

  auto context = std::make_unique<grpc::ClientContext>();
  context->set_deadline(
      engine::Deadline::FromDuration(utest::kMaxTestWaitTime));

  ugrpc::client::Qos qos;
  qos.timeout = tests::kShortTimeout;
  auto call = client.SayHello(kRequest, std::move(context), qos);

  sample::ugrpc::GreetingResponse response;
  UEXPECT_THROW(response = call.Finish(), ugrpc::client::DeadlineExceededError);
}

UTEST_F(GrpcClientQosConfigExceeded, EmptyConfigMeansInfinity) {
  auto client =
      GetClientFactory().MakeClient<sample::ugrpc::UnitTestServiceClient>(
          MakeClientSettingsWithQos());

  // Note: __default__ is not defined.
  ExtendDynamicConfig({{tests::kUnitTestClientQos, {}}});
  tests::InitTaskInheritedDeadline(
      engine::Deadline::FromDuration(tests::kShortTimeout));

  auto call = client.SayHello(kRequest);

  sample::ugrpc::GreetingResponse response;
  UEXPECT_THROW(response = call.Finish(), ugrpc::client::DeadlineExceededError);
}

USERVER_NAMESPACE_END
