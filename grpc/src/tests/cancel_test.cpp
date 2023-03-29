#include <userver/utest/utest.hpp>

#include <userver/engine/async.hpp>
#include <userver/engine/single_consumer_event.hpp>
#include <userver/engine/sleep.hpp>

#include <userver/ugrpc/client/client_factory.hpp>
#include <userver/ugrpc/client/exceptions.hpp>
#include <userver/ugrpc/client/queue_holder.hpp>
#include <userver/ugrpc/server/server.hpp>

#include <tests/service_fixture_test.hpp>
#include <tests/unit_test_client.usrv.pb.hpp>
#include <tests/unit_test_service.usrv.pb.hpp>

USERVER_NAMESPACE_BEGIN

using namespace std::chrono_literals;

namespace {

class UnitTestServiceCancelEcho final
    : public sample::ugrpc::UnitTestServiceBase {
 public:
  void Chat(ChatCall& call) override {
    sample::ugrpc::StreamGreetingRequest request;
    ASSERT_TRUE(call.Read(request));
    UASSERT_NO_THROW(call.Write({}));

    ASSERT_FALSE(call.Read(request));
    UASSERT_THROW(call.Finish(), ugrpc::server::RpcInterruptedError);
  }
};

}  // namespace

using GrpcCancel = GrpcServiceFixtureSimple<UnitTestServiceCancelEcho>;

UTEST_F(GrpcCancel, TryCancel) {
  auto client = MakeClient<sample::ugrpc::UnitTestServiceClient>();

  for (int i = 0; i < 2; ++i) {
    auto call = client.Chat();

    EXPECT_TRUE(call.Write({}));
    sample::ugrpc::StreamGreetingResponse response;
    EXPECT_TRUE(call.Read(response));

    // Drop 'call' without finishing. After this the server side should
    // immediately receive RpcInterruptedError. The connection should not be
    // closed.
  }
}

namespace {

class UnitTestServiceEcho final : public sample::ugrpc::UnitTestServiceBase {
 public:
  void Chat(ChatCall& call) override {
    sample::ugrpc::StreamGreetingRequest request;
    sample::ugrpc::StreamGreetingResponse response;
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

UTEST_MT(GrpcServer, DestroyServerDuringRequest, 2) {
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

  const std::string endpoint = fmt::format("[::1]:{}", server.GetPort());
  auto client =
      client_factory.MakeClient<sample::ugrpc::UnitTestServiceClient>(endpoint);

  auto call = client.Chat();
  // NOLINTNEXTLINE(clang-analyzer-optin.cplusplus.UninitializedObject)
  EXPECT_TRUE(call.Write({}));

  sample::ugrpc::StreamGreetingResponse response;
  EXPECT_TRUE(call.Read(response));

  auto complete_rpc = engine::AsyncNoSpan([&] {
    // Make sure that 'server.Stop' call starts
    engine::SleepFor(50ms);

    // The server should wait for the ongoing RPC to complete
    EXPECT_TRUE(call.Write({}));
    UEXPECT_NO_THROW(EXPECT_TRUE(call.Read(response)));
    EXPECT_TRUE(call.WritesDone());
    UEXPECT_NO_THROW(EXPECT_FALSE(call.Read(response)));
  });

  server.Stop();
  complete_rpc.Get();
}

UTEST(GrpcServer, DeadlineAffectsWaitForReady) {
  utils::statistics::Storage statistics_storage;
  ugrpc::client::QueueHolder client_queue;
  const std::string endpoint = "[::1]:1234";

  ugrpc::client::ClientFactory client_factory(
      ugrpc::client::ClientFactoryConfig{},
      engine::current_task::GetTaskProcessor(), client_queue.GetQueue(),
      statistics_storage);

  auto client =
      client_factory.MakeClient<sample::ugrpc::UnitTestServiceClient>(endpoint);

  auto context = std::make_unique<grpc::ClientContext>();
  context->set_deadline(engine::Deadline::FromDuration(100ms));
  context->set_wait_for_ready(true);

  auto long_deadline = engine::Deadline::FromDuration(100ms + 1s);
  auto call = client.SayHello({}, std::move(context));
  UEXPECT_THROW(call.Finish(), ugrpc::client::DeadlineExceededError);
  EXPECT_FALSE(long_deadline.IsReached());
}

namespace {

class UnitTestServiceCancelHello final
    : public sample::ugrpc::UnitTestServiceBase {
 public:
  UnitTestServiceCancelHello() = default;

  void SayHello(SayHelloCall& call,
                ::sample::ugrpc::GreetingRequest&&) override {
    const sample::ugrpc::GreetingResponse response;

    // Wait until cancelled.
    const bool success = wait_event_.WaitForEvent();

    finish_event_.Send();
    call.Finish(response);
    EXPECT_FALSE(success);
    EXPECT_TRUE(engine::current_task::ShouldCancel());
  }

  auto& GetWaitEvent() { return wait_event_; }
  auto& GetFinishEvent() { return finish_event_; }

 private:
  engine::SingleConsumerEvent wait_event_;
  engine::SingleConsumerEvent finish_event_;
};

}  // namespace

using GrpcCancelByClient = GrpcServiceFixtureSimple<UnitTestServiceCancelHello>;

UTEST_F_MT(GrpcCancelByClient, CancelByClient, 3) {
  auto client = MakeClient<sample::ugrpc::UnitTestServiceClient>();

  auto context = std::make_unique<grpc::ClientContext>();
  context->set_deadline(engine::Deadline::FromDuration(100ms));
  context->set_wait_for_ready(true);
  auto call = client.SayHello({}, std::move(context));
  EXPECT_THROW(call.Finish(), ugrpc::client::BaseError);

  ASSERT_TRUE(
      GetService().GetFinishEvent().WaitForEventFor(std::chrono::seconds{5}));
}

USERVER_NAMESPACE_END
