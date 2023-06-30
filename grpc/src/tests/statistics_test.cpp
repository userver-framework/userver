#include <userver/utest/utest.hpp>

#include <userver/engine/get_all.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/utils/async.hpp>

#include <userver/ugrpc/client/exceptions.hpp>

#include <tests/service_fixture_test.hpp>
#include <tests/unit_test_client.usrv.pb.hpp>
#include <tests/unit_test_service.usrv.pb.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

class UnitTestServiceForStatistics final
    : public sample::ugrpc::UnitTestServiceBase {
 public:
  void SayHello(SayHelloCall& call, sample::ugrpc::GreetingRequest&&) override {
    engine::SleepFor(std::chrono::milliseconds{20});
    call.FinishWithError(
        {grpc::StatusCode::INVALID_ARGUMENT, "message", "details"});
  }

  void Chat(ChatCall& call) override {
    call.FinishWithError(
        {grpc::StatusCode::UNIMPLEMENTED, "message", "details"});
  }
};

}  // namespace

using GrpcStatistics = GrpcServiceFixtureSimple<UnitTestServiceForStatistics>;

UTEST_F(GrpcStatistics, LongRequest) {
  auto client = MakeClient<sample::ugrpc::UnitTestServiceClient>();
  sample::ugrpc::GreetingRequest out;
  out.set_name("userver");
  UEXPECT_THROW(client.SayHello(out).Finish(),
                ugrpc::client::InvalidArgumentError);
  GetServer().StopDebug();

  for (const auto& domain : {"client", "server"}) {
    const auto stats = GetStatistics(
        fmt::format("grpc.{}.by-destination", domain),
        {{"grpc_destination", "sample.ugrpc.UnitTestService/SayHello"}});

    const auto get_status_code_count_legacy = [&](const std::string& code) {
      return stats.SingleMetric("status", {{"grpc_code", code}}).AsInt();
    };
    const auto get_status_code_count = [&](const std::string& code) {
      return stats.SingleMetric("status.v2", {{"grpc_code", code}})
          .AsRate()
          .value;
    };
    EXPECT_EQ(get_status_code_count("OK"), 0);
    EXPECT_EQ(get_status_code_count("INVALID_ARGUMENT"), 1);
    EXPECT_EQ(get_status_code_count("ALREADY_EXISTS"), 0);
    EXPECT_EQ(stats.SingleMetric("rps.v2").AsRate().value, 1);
    EXPECT_EQ(stats.SingleMetric("eps.v2").AsRate().value, 1);
    EXPECT_EQ(stats.SingleMetric("network-error.v2").AsRate().value, 0);
    EXPECT_EQ(stats.SingleMetric("abandoned-error.v2").AsRate().value, 0);

    // check that legacy stats is still collected
    EXPECT_EQ(get_status_code_count_legacy("OK"), 0);
    EXPECT_EQ(get_status_code_count_legacy("INVALID_ARGUMENT"), 1);
    EXPECT_EQ(get_status_code_count_legacy("ALREADY_EXISTS"), 0);
    EXPECT_EQ(stats.SingleMetric("rps").AsInt(), 1);
    EXPECT_EQ(stats.SingleMetric("eps").AsInt(), 1);
    EXPECT_EQ(stats.SingleMetric("network-error").AsInt(), 0);
    EXPECT_EQ(stats.SingleMetric("abandoned-error").AsInt(), 0);
  }
}

UTEST_F_MT(GrpcStatistics, Multithreaded, 2) {
  constexpr int kIterations = 10;

  auto client = MakeClient<sample::ugrpc::UnitTestServiceClient>();

  auto say_hello_task = utils::Async("say-hello", [&] {
    for (int i = 0; i < kIterations; ++i) {
      sample::ugrpc::GreetingRequest out;
      out.set_name("userver");
      UEXPECT_THROW(client.SayHello(out).Finish(),
                    ugrpc::client::InvalidArgumentError);
    }
  });

  auto chat_task = utils::Async("chat", [&] {
    for (int i = 0; i < kIterations; ++i) {
      auto chat = client.Chat();
      sample::ugrpc::StreamGreetingResponse response;
      UEXPECT_THROW((void)chat.Read(response),
                    ugrpc::client::UnimplementedError);
    }
  });

  engine::GetAll(say_hello_task, chat_task);
  GetServer().StopDebug();

  for (const auto& domain : {"client", "server"}) {
    const auto status =
        GetStatistics(fmt::format("grpc.{}.by-destination.status", domain));
    const utils::statistics::Label say_hello_label{
        "grpc_destination", "sample.ugrpc.UnitTestService/SayHello"};
    const utils::statistics::Label chat_label{
        "grpc_destination", "sample.ugrpc.UnitTestService/Chat"};

    const auto get_status_code_count = [&](const auto& label, auto code) {
      return status.SingleMetric("", {label, {"grpc_code", code}}).AsInt();
    };

    const auto say_hello_invalid_argument =
        get_status_code_count(say_hello_label, "INVALID_ARGUMENT");
    EXPECT_EQ(say_hello_invalid_argument, kIterations);

    EXPECT_EQ(get_status_code_count(say_hello_label, "UNIMPLEMENTED"), 0);
    EXPECT_EQ(get_status_code_count(chat_label, "INVALID_ARGUMENT"), 0);
    EXPECT_EQ(get_status_code_count(chat_label, "UNIMPLEMENTED"), kIterations);
  }
}

USERVER_NAMESPACE_END
