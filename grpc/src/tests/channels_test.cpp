#include <userver/ugrpc/client/channels.hpp>

#include <userver/engine/async.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/engine/task/task.hpp>
#include <userver/utest/utest.hpp>
#include <userver/utils/statistics/storage.hpp>

#include <userver/ugrpc/client/client_factory.hpp>
#include <userver/ugrpc/client/queue_holder.hpp>
#include <userver/ugrpc/server/server.hpp>

#include "unit_test_client.usrv.pb.hpp"
#include "unit_test_service.usrv.pb.hpp"

USERVER_NAMESPACE_BEGIN

using namespace std::chrono_literals;
using namespace sample::ugrpc;

namespace {

class UnitTestServiceSimple final : public UnitTestServiceBase {
 public:
  void SayHello(SayHelloCall& call, GreetingRequest&& request) override {
    call.Finish({});
  }
};

constexpr int kPort = 12345;

ugrpc::server::ServerConfig MakeServerConfig() {
  ugrpc::server::ServerConfig config;
  config.port = kPort;
  return config;
}

}  // namespace

UTEST_MT(GrpcChannels, TryWaitForConnected, 2) {
  constexpr auto kSmallTimeout = 100ms;
  constexpr auto kServerStartDelay = 100ms;
  constexpr auto kMaxServerStartTime = 500ms;
  utils::statistics::Storage statistics_storage;

  auto client_task = engine::AsyncNoSpan([&] {
    ugrpc::client::ClientFactoryConfig config;
    config.channel_args.SetInt("grpc.testing.fixed_reconnect_backoff_ms", 100);
    ugrpc::client::QueueHolder client_queue;

    ugrpc::client::ClientFactory client_factory(
        std::move(config), engine::current_task::GetTaskProcessor(),
        client_queue.GetQueue(), statistics_storage);

    const auto endpoint = fmt::format("[::1]:{}", kPort);
    auto client = client_factory.MakeClient<UnitTestServiceClient>(endpoint);

    // TryWaitForConnected should wait for the server to start and return 'true'
    EXPECT_TRUE(ugrpc::client::TryWaitForConnected(
        client,
        engine::Deadline::FromDuration(kServerStartDelay + kMaxServerStartTime),
        engine::current_task::GetTaskProcessor()));

    auto call = client.SayHello({});
    UEXPECT_NO_THROW((void)call.Finish());

    // TryWaitForConnected should return immediately if the connection is
    // already alive
    EXPECT_TRUE(ugrpc::client::TryWaitForConnected(
        client, engine::Deadline::FromDuration(kSmallTimeout),
        engine::current_task::GetTaskProcessor()));
  });

  // Make sure that TryWaitForConnected starts while the server is down
  engine::SleepFor(kServerStartDelay);

  UnitTestServiceSimple service;
  ugrpc::server::Server server(MakeServerConfig(), statistics_storage);
  server.AddService(service, engine::current_task::GetTaskProcessor());
  server.Start();
  client_task.Get();
  server.Stop();
}

USERVER_NAMESPACE_END
