#include <userver/utest/utest.hpp>

#include <userver/engine/get_all.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/utils/async.hpp>

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

using GrpcStatistics = GrpcServiceFixtureSimple<UnitTestServiceForStatistics>;

UTEST_F(GrpcStatistics, LongRequest) {
  auto client = MakeClient<UnitTestServiceClient>();
  GreetingRequest out;
  out.set_name("userver");
  UEXPECT_THROW(client.SayHello(out).Finish(),
                ugrpc::client::InvalidArgumentError);
  GetServer().StopDebug();

  const auto statistics = GetStatistics();
  for (const auto& domain : {"client", "server"}) {
    const auto hello_statistics =
        statistics["grpc"][domain]["sample.ugrpc.UnitTestService"]["SayHello"];
    EXPECT_EQ(hello_statistics["status"]["OK"].As<int>(), 0);
    EXPECT_EQ(hello_statistics["status"]["INVALID_ARGUMENT"].As<int>(), 1);
    EXPECT_EQ(hello_statistics["status"]["ALREADY_EXISTS"].As<int>(), 0);
    EXPECT_EQ(hello_statistics["rps"].As<int>(), 1);
    EXPECT_EQ(hello_statistics["eps"].As<int>(), 1);
    EXPECT_EQ(hello_statistics["network-error"].As<int>(), 0);
    EXPECT_EQ(hello_statistics["abandoned-error"].As<int>(), 0);
  }
}

UTEST_F_MT(GrpcStatistics, Multithreaded, 2) {
  constexpr int kIterations = 10;

  auto client = MakeClient<UnitTestServiceClient>();

  auto say_hello_task = utils::Async("say-hello", [&] {
    for (int i = 0; i < kIterations; ++i) {
      GreetingRequest out;
      out.set_name("userver");
      UEXPECT_THROW(client.SayHello(out).Finish(),
                    ugrpc::client::InvalidArgumentError);
    }
  });

  auto chat_task = utils::Async("chat", [&] {
    for (int i = 0; i < kIterations; ++i) {
      auto chat = client.Chat();
      StreamGreetingResponse response;
      UEXPECT_THROW((void)chat.Read(response),
                    ugrpc::client::UnimplementedError);
    }
  });

  engine::GetAll(say_hello_task, chat_task);
  GetServer().StopDebug();

  const auto statistics = GetStatistics();
  for (const auto& domain : {"client", "server"}) {
    const auto service_statistics =
        statistics["grpc"][domain]["sample.ugrpc.UnitTestService"];
    const auto say_hello_statistics = service_statistics["SayHello"];
    const auto chat_statistics = service_statistics["Chat"];

    // TODO(TAXICOMMON-5134) It must always be equal to kIterations
    //  Maybe investigate overall statistics on failure?
    const auto say_hello_invalid_argument =
        say_hello_statistics["status"]["INVALID_ARGUMENT"].As<int>();
    EXPECT_GE(say_hello_invalid_argument, 0);
    EXPECT_LE(say_hello_invalid_argument, kIterations);

    EXPECT_EQ(say_hello_statistics["status"]["UNIMPLEMENTED"].As<int>(), 0);
    EXPECT_EQ(chat_statistics["status"]["INVALID_ARGUMENT"].As<int>(), 0);
    EXPECT_EQ(chat_statistics["status"]["UNIMPLEMENTED"].As<int>(),
              kIterations);
  }
}

USERVER_NAMESPACE_END
