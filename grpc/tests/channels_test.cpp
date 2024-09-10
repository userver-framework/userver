#include <userver/ugrpc/client/channels.hpp>

#include <userver/dynamic_config/storage_mock.hpp>
#include <userver/dynamic_config/test_helpers.hpp>
#include <userver/engine/async.hpp>
#include <userver/engine/io/socket.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/engine/task/task.hpp>
#include <userver/logging/null_logger.hpp>
#include <userver/utest/utest.hpp>
#include <userver/utils/statistics/storage.hpp>

#include <../include/userver/ugrpc/client/impl/completion_queue_pool.hpp>
#include <userver/ugrpc/client/client_factory.hpp>
#include <userver/ugrpc/server/server.hpp>
#include <userver/ugrpc/tests/service.hpp>
#include <userver/ugrpc/tests/standalone_client.hpp>

#include <tests/unit_test_client.usrv.pb.hpp>
#include <tests/unit_test_service.usrv.pb.hpp>

using namespace std::chrono_literals;

USERVER_NAMESPACE_BEGIN

namespace {

class UnitTestServiceSimple final : public sample::ugrpc::UnitTestServiceBase {
 public:
  void SayHello(SayHelloCall& call, sample::ugrpc::GreetingRequest&&) override {
    sample::ugrpc::GreetingResponse response{};
    call.Finish(response);
    EXPECT_FALSE(engine::current_task::ShouldCancel());
  }
};

ugrpc::server::ServerConfig MakeServerConfig(int port) {
  ugrpc::server::ServerConfig config;
  config.port = port;
  return config;
}

}  // namespace

struct GrpcChannels : public ::testing::TestWithParam<std::size_t> {};

UTEST_P_MT(GrpcChannels, TryWaitForConnected, 2) {
  constexpr auto kSmallTimeout = 100ms;
  constexpr auto kServerStartDelay = 100ms;

  const auto port = ugrpc::tests::GetFreeIpv6Port();

  auto client_task = engine::AsyncNoSpan([&] {
    ugrpc::client::ClientFactorySettings settings;
    settings.channel_args.SetInt("grpc.testing.fixed_reconnect_backoff_ms",
                                 100);
    settings.channel_count = GetParam();

    ugrpc::tests::StandaloneClientFactory client_factory{std::move(settings)};

    const auto endpoint = ugrpc::tests::MakeIpv6Endpoint(port);
    auto client =
        client_factory.MakeClient<sample::ugrpc::UnitTestServiceClient>(
            endpoint);

    // TryWaitForConnected should wait for the server to start and return 'true'
    EXPECT_TRUE(ugrpc::client::TryWaitForConnected(
        client, engine::Deadline::FromDuration(utest::kMaxTestWaitTime),
        engine::current_task::GetTaskProcessor()));

    auto call = client.SayHello({});
    EXPECT_FALSE(engine::current_task::ShouldCancel());
    UEXPECT_NO_THROW((void)call.Finish());
    EXPECT_FALSE(engine::current_task::ShouldCancel());

    // TryWaitForConnected should return immediately if the connection is
    // already alive
    EXPECT_TRUE(ugrpc::client::TryWaitForConnected(
        client, engine::Deadline::FromDuration(kSmallTimeout),
        engine::current_task::GetTaskProcessor()));
  });

  // Make sure that TryWaitForConnected starts while the server is down
  engine::SleepFor(kServerStartDelay);

  const ugrpc::tests::Service<UnitTestServiceSimple> service{
      MakeServerConfig(port)};

  UEXPECT_NO_THROW(client_task.Get());
}

INSTANTIATE_UTEST_SUITE_P(Basic, GrpcChannels,
                          ::testing::Values(std::size_t{1}, std::size_t{4}));

USERVER_NAMESPACE_END
