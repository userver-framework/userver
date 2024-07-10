#include <greeter_service.hpp>

#include <userver/ugrpc/tests/service_fixtures.hpp>
#include <userver/utest/using_namespace_userver.hpp>
#include <userver/utest/utest.hpp>

#include <samples/greeter_client.usrv.pb.hpp>

#include <greeter_client.hpp>
#include <greeter_service.hpp>

namespace {

/// [service fixture]
class GreeterServiceTest : public ugrpc::tests::ServiceFixtureBase {
 protected:
  GreeterServiceTest() : service_(prefix_) {
    RegisterService(service_);
    StartServer();
  }

  ~GreeterServiceTest() override { StopServer(); }

 private:
  const std::string prefix_{"Hello"};
  // We've made sure to separate the logic into samples::GreeterService that is
  // detached from the component system and only depends on things obtainable
  // in gtest tests.
  samples::GreeterService service_;
};
/// [service fixture]

}  // namespace

/// [service tests]
UTEST_F(GreeterServiceTest, DirectCall) {
  const auto client = MakeClient<samples::api::GreeterServiceClient>();

  samples::api::GreetingRequest request;
  request.set_name("gtest");
  const auto response = client.SayHello(request).Finish();

  EXPECT_EQ(response.greeting(), "Hello, gtest!");
}

UTEST_F(GreeterServiceTest, CustomClient) {
  // We've made sure to separate some logic into samples::GreeterClient that is
  // detached from the component system, it only needs the gRPC client, which we
  // can create in gtest tests.
  const samples::GreeterClient client{
      MakeClient<samples::api::GreeterServiceClient>()};

  const auto response = client.SayHello("gtest");
  EXPECT_EQ(response, "Hello, gtest!");
}
/// [service tests]

/// [client tests]
namespace {

class GreeterMock final : public samples::api::GreeterServiceBase {
 public:
  void SayHello(SayHelloCall& call,
                samples::api::GreetingRequest&& /*request*/) override {
    samples::api::GreetingResponse response;
    response.set_greeting("Mocked response");
    call.Finish(response);
  }
};

// Default-constructs GreeterMock.
using GreeterClientTest = ugrpc::tests::ServiceFixture<GreeterMock>;

}  // namespace

UTEST_F(GreeterClientTest, MockedServiceCustomClient) {
  const samples::GreeterClient client{
      MakeClient<samples::api::GreeterServiceClient>()};

  const auto response = client.SayHello("gtest");
  EXPECT_EQ(response, "Mocked response");
}
/// [client tests]
