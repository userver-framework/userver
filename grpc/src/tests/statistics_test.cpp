#include <userver/utest/utest.hpp>

#include <userver/engine/sleep.hpp>

#include <userver/ugrpc/client/exceptions.hpp>

#include <tests/service_fixture_test.hpp>
#include "unit_test_client.usrv.pb.hpp"
#include "unit_test_service.usrv.pb.hpp"

using namespace sample::ugrpc;

USERVER_NAMESPACE_BEGIN

namespace {

class UnitTestServiceForStatistics final : public UnitTestServiceBase {
 public:
  void SayHello(SayHelloCall& call, GreetingRequest&& request) override {
    engine::SleepFor(std::chrono::milliseconds{20});
    call.FinishWithError(
        {grpc::StatusCode::INVALID_ARGUMENT, "message", "details"});
  }
};

}  // namespace

using GrpcServerStatistics =
    GrpcServiceFixtureSimple<UnitTestServiceForStatistics>;

UTEST_F(GrpcServerStatistics, LongRequest) {
  auto client = MakeClient<UnitTestServiceClient>();
  GreetingRequest out;
  out.set_name("userver");
  EXPECT_THROW(client.SayHello(out).Finish(),
               ugrpc::client::InvalidArgumentError);

  const auto statistics = GetStatistics();
  const auto hello_statistics =
      statistics["grpc"]["server"]["sample.ugrpc.UnitTestService"]["SayHello"];
  EXPECT_EQ(hello_statistics["status"]["OK"].As<int>(), 0);
  EXPECT_EQ(hello_statistics["status"]["INVALID_ARGUMENT"].As<int>(), 1);
  EXPECT_EQ(hello_statistics["status"]["ALREADY_EXISTS"].As<int>(), 0);
  EXPECT_EQ(hello_statistics["rps"].As<int>(), 1);
  EXPECT_EQ(hello_statistics["eps"].As<int>(), 1);
  EXPECT_EQ(hello_statistics["network-error"].As<int>(), 0);
  EXPECT_EQ(hello_statistics["internal-error"].As<int>(), 0);
}

USERVER_NAMESPACE_END
