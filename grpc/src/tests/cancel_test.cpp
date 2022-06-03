#include <userver/utest/utest.hpp>

#include <userver/engine/async.hpp>
#include <userver/engine/sleep.hpp>

#include <userver/ugrpc/client/client_factory.hpp>
#include <userver/ugrpc/client/exceptions.hpp>
#include <userver/ugrpc/client/queue_holder.hpp>
#include <userver/ugrpc/server/server.hpp>

#include <tests/service_fixture_test.hpp>
#include "unit_test_client.usrv.pb.hpp"
#include "unit_test_service.usrv.pb.hpp"

USERVER_NAMESPACE_BEGIN

using namespace std::chrono_literals;
using namespace sample::ugrpc;

namespace {

class UnitTestServiceCancelEcho final : public UnitTestServiceBase {
 public:
  void Chat(ChatCall& call) override {
    StreamGreetingRequest request;
    ASSERT_TRUE(call.Read(request));
    UASSERT_NO_THROW(call.Write({}));

    ASSERT_FALSE(call.Read(request));
    UASSERT_THROW(call.Finish(), ugrpc::server::RpcInterruptedError);
  }
};

}  // namespace

using GrpcCancel = GrpcServiceFixtureSimple<UnitTestServiceCancelEcho>;

UTEST_F(GrpcCancel, TryCancel) {
  auto client = MakeClient<UnitTestServiceClient>();

  for (int i = 0; i < 2; ++i) {
    auto call = client.Chat();

    UEXPECT_NO_THROW(call.Write({}));
    StreamGreetingResponse response;
    EXPECT_TRUE(call.Read(response));

    // Drop 'call' without finishing. After this the server side should
    // immediately receive RpcInterruptedError. The connection should not be
    // closed.
  }
}

namespace {

class UnitTestServiceEcho final : public UnitTestServiceBase {
 public:
  void Chat(ChatCall& call) override {
    StreamGreetingRequest request;
    StreamGreetingResponse response;
    while (call.Read(request)) {
      response.set_name(request.name());
      response.set_number(request.number());
      call.Write(response);
    }
    call.Finish();
  }
};

ugrpc::server::ServerConfig MakeServerConfig() {
  ugrpc::server::ServerConfig config;
  config.port = 0;
  return config;
}

}  // namespace

UTEST_MT(GrpcServer, DestroyServerDuringReqest, 2) {
  UnitTestServiceEcho service;
  utils::statistics::Storage statistics_storage;

  ugrpc::server::Server server(MakeServerConfig(), statistics_storage);
  server.AddService(service, engine::current_task::GetTaskProcessor());
  server.Start();

  // A separate client queue is necessary, since the client stops after the
  // server in this test.
  ugrpc::client::QueueHolder client_queue;

  ugrpc::client::ClientFactory client_factory(
      ugrpc::client::ClientFactoryConfig{},
      engine::current_task::GetTaskProcessor(), client_queue.GetQueue(),
      statistics_storage);

  const auto endpoint = fmt::format("[::1]:{}", server.GetPort());
  auto client = client_factory.MakeClient<UnitTestServiceClient>(endpoint);

  auto call = client.Chat();
  call.Write({});

  StreamGreetingResponse response;
  EXPECT_TRUE(call.Read(response));

  auto complete_rpc = engine::AsyncNoSpan([&] {
    // Make sure that 'server.Stop' call starts
    engine::SleepFor(50ms);

    // The server should wait for the ongoing RPC to complete
    call.Write({});
    EXPECT_TRUE(call.Read(response));
    UEXPECT_NO_THROW(call.Finish());
  });

  server.Stop();
  complete_rpc.Get();
}

UTEST(GrpcServer, DeadlineAffectsWaitForReady) {
  utils::statistics::Storage statistics_storage;
  ugrpc::client::QueueHolder client_queue;
  const auto endpoint = "[::1]:1234";

  ugrpc::client::ClientFactory client_factory(
      ugrpc::client::ClientFactoryConfig{},
      engine::current_task::GetTaskProcessor(), client_queue.GetQueue(),
      statistics_storage);

  auto client = client_factory.MakeClient<UnitTestServiceClient>(endpoint);

  auto context = std::make_unique<grpc::ClientContext>();
  context->set_deadline(engine::Deadline::FromDuration(100ms));
  context->set_wait_for_ready(true);

  auto long_deadline = engine::Deadline::FromDuration(100ms + 1s);
  auto call = client.SayHello({}, std::move(context));
  UEXPECT_THROW(call.Finish(), ugrpc::client::DeadlineExceededError);
  EXPECT_FALSE(long_deadline.IsReached());
}

USERVER_NAMESPACE_END
