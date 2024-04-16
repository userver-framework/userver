#include <gtest/gtest.h>

#include <chrono>
#include <string_view>

#include <userver/engine/async.hpp>
#include <userver/engine/deadline.hpp>
#include <userver/engine/future_status.hpp>
#include <userver/engine/single_consumer_event.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/engine/task/task_with_result.hpp>
#include <userver/server/request/task_inherited_data.hpp>
#include <userver/utils/algo.hpp>

#include <ugrpc/client/impl/client_configs.hpp>
#include <ugrpc/client/middlewares/deadline_propagation/middleware.hpp>
#include <ugrpc/server/impl/server_configs.hpp>
#include <ugrpc/server/middlewares/deadline_propagation/middleware.hpp>
#include <userver/ugrpc/client/exceptions.hpp>
#include <userver/ugrpc/client/queue_holder.hpp>

#include <tests/deadline_helpers.hpp>
#include <tests/unit_test_client.usrv.pb.hpp>
#include <tests/unit_test_service.usrv.pb.hpp>
#include <userver/ugrpc/tests/service_fixtures.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

template <typename Call, typename Response>
void CheckSuccessRead(Call& call, Response& response, std::string_view result) {
  bool res = false;
  UEXPECT_NO_THROW(res = call.Read(response));
  EXPECT_EQ(result, response.name());
  EXPECT_TRUE(res);
}

template <typename Call, typename Request>
void CheckSuccessWrite(Call& call, Request& request, const char* message) {
  bool res = false;
  request.set_name(message);
  // NOLINTNEXTLINE(clang-analyzer-optin.cplusplus.UninitializedObject)
  UEXPECT_NO_THROW(res = call.Write(request));
  EXPECT_TRUE(res);
}

constexpr const char* kRequests[] = {
    "First userver",   //
    "Second userver",  //
    "Third userver"    //
};

constexpr const char* kResponses[] = {
    "One First userver",   //
    "Two Second userver",  //
    "Three Third userver"  //
};

class UnitTestDeadlinePropagationService final
    : public sample::ugrpc::UnitTestServiceBase {
 public:
  void SayHello(SayHelloCall& call,
                sample::ugrpc::GreetingRequest&& request) override {
    sample::ugrpc::GreetingResponse response;
    response.set_name("Hello " + request.name());

    tests::WaitUntilRpcDeadline(call);

    call.Finish(response);
  }

  void ReadMany(ReadManyCall& call,
                ::sample::ugrpc::StreamGreetingRequest&& request) override {
    sample::ugrpc::StreamGreetingResponse response;
    response.set_name("One " + request.name());
    // NOLINTNEXTLINE(clang-analyzer-optin.cplusplus.UninitializedObject)
    call.Write(response);
    response.set_name("Two " + request.name());
    // NOLINTNEXTLINE(clang-analyzer-optin.cplusplus.UninitializedObject)
    call.Write(response);
    response.set_name("Three " + request.name());
    // NOLINTNEXTLINE(clang-analyzer-optin.cplusplus.UninitializedObject)
    call.Write(response);

    tests::WaitUntilRpcDeadline(call);
    call.Finish();
  }

  void WriteMany(WriteManyCall& call) override {
    sample::ugrpc::StreamGreetingRequest request;
    std::size_t reads{0};

    while (call.Read(request)) {
      ASSERT_LT(reads, std::size(kRequests));
      EXPECT_EQ(request.name(), kRequests[reads]);
      ++reads;
    }

    sample::ugrpc::StreamGreetingResponse response;
    response.set_name("Hello " + request.name());

    tests::WaitUntilRpcDeadline(call);

    call.Finish(response);
  }

  void Chat(ChatCall& call) override {
    std::vector<sample::ugrpc::StreamGreetingRequest> requests(3);

    sample::ugrpc::StreamGreetingResponse response;

    for (auto& it : requests) {
      if (!call.Read(it)) {
        // It is deadline from client side
        return;
      }
    }

    response.set_name("One " + requests[0].name());
    UEXPECT_NO_THROW(call.Write(response));
    response.set_name("Two " + requests[1].name());
    UEXPECT_NO_THROW(call.Write(response));

    tests::WaitUntilRpcDeadline(call);

    response.set_name("Three " + requests[0].name());
    UEXPECT_THROW(call.WriteAndFinish(response),
                  ugrpc::server::RpcInterruptedError);
  }
};

class GrpcDeadlinePropagation
    : public ugrpc::tests::ServiceFixture<UnitTestDeadlinePropagationService> {
 public:
  using ClientType = sample::ugrpc::UnitTestServiceClient;

  GrpcDeadlinePropagation()
      : client_deadline_(engine::Deadline::FromDuration(tests::kShortTimeout)),
        long_deadline_(engine::Deadline::FromDuration(tests::kLongTimeout)),
        client_(MakeClient<ClientType>()) {
    tests::InitTaskInheritedDeadline(client_deadline_);
  }

  ClientType& Client() { return client_; }

  void WaitClientDeadline() {
    tests::WaitUntilRpcDeadlineClient(client_deadline_);
  }

  ~GrpcDeadlinePropagation() override {
    EXPECT_TRUE(client_deadline_.IsReached());
    EXPECT_FALSE(long_deadline_.IsReached());
  }

 private:
  engine::Deadline client_deadline_;
  engine::Deadline long_deadline_;
  ClientType client_;
};

}  // namespace

UTEST_F(GrpcDeadlinePropagation, TestClientUnaryCallFinish) {
  sample::ugrpc::GreetingRequest request;
  request.set_name("userver");

  auto context = std::make_unique<grpc::ClientContext>();
  auto call = Client().SayHello(request, std::move(context));

  sample::ugrpc::GreetingResponse in;
  UEXPECT_THROW(in = call.Finish(), ugrpc::client::DeadlineExceededError);
}

UTEST_F(GrpcDeadlinePropagation, TestClientUnaryCallFinishAsync) {
  sample::ugrpc::GreetingRequest request;
  request.set_name("userver");

  auto context = std::make_unique<grpc::ClientContext>();
  auto call = Client().SayHello(request, std::move(context));

  sample::ugrpc::GreetingResponse in;
  auto future = call.FinishAsync(in);
  UEXPECT_THROW(future.Get(), ugrpc::client::DeadlineExceededError);
}

UTEST_F(GrpcDeadlinePropagation, TestClientUnaryCallFinishAsyncWaitUntil) {
  sample::ugrpc::GreetingRequest request;
  request.set_name("userver");

  auto context = std::make_unique<grpc::ClientContext>();
  context->set_deadline(engine::Deadline::FromDuration(tests::kShortTimeout));
  auto deadline = engine::Deadline::FromDuration(tests::kShortTimeout / 100);

  auto call = Client().SayHello(request, std::move(context));

  sample::ugrpc::GreetingResponse in;
  auto future = call.FinishAsync(in);
  EXPECT_EQ(future.Get(deadline), engine::FutureStatus::kTimeout);

  auto result = engine::FutureStatus::kReady;
  UEXPECT_THROW(result = future.Get(
                    engine::Deadline::FromDuration(2 * tests::kShortTimeout)),
                ugrpc::client::DeadlineExceededError);
}

UTEST_F(GrpcDeadlinePropagation, TestClientReadManyRead) {
  sample::ugrpc::StreamGreetingRequest request;
  request.set_name("userver");

  auto context = std::make_unique<grpc::ClientContext>();
  auto call = Client().ReadMany(request, std::move(context));

  sample::ugrpc::StreamGreetingResponse response;
  bool res = false;

  CheckSuccessRead(call, response, "One userver");
  CheckSuccessRead(call, response, "Two userver");
  CheckSuccessRead(call, response, "Three userver");

  UEXPECT_THROW(res = call.Read(response),
                ugrpc::client::DeadlineExceededError);
}

UTEST_F(GrpcDeadlinePropagation, TestClientWriteManyWriteAndCheck) {
  sample::ugrpc::StreamGreetingRequest request;

  auto context = std::make_unique<grpc::ClientContext>();
  auto call = Client().WriteMany(std::move(context));

  sample::ugrpc::StreamGreetingResponse response;

  CheckSuccessWrite(call, request, "First userver");
  CheckSuccessWrite(call, request, "Second userver");

  WaitClientDeadline();

  request.set_name("Third userver");
  UEXPECT_THROW(call.WriteAndCheck(request),
                ugrpc::client::DeadlineExceededError);
}

UTEST_F(GrpcDeadlinePropagation, TestClientWriteManyFinish) {
  sample::ugrpc::StreamGreetingRequest request;
  auto context = std::make_unique<grpc::ClientContext>();
  auto call = Client().WriteMany(std::move(context));

  sample::ugrpc::StreamGreetingResponse response;

  CheckSuccessWrite(call, request, "First userver");
  CheckSuccessWrite(call, request, "Second userver");
  CheckSuccessWrite(call, request, "Third userver");

  UEXPECT_THROW(response = call.Finish(), ugrpc::client::DeadlineExceededError);
}

// Scenario for Chat tests (Read, ReadAsync, Write, WriteAndCheck):
// Client Write x3, WritesDone
// Server Read x3, Write x2, Time consuming calculations, Write
// Client Read x3

UTEST_F(GrpcDeadlinePropagation, TestClientChatWrite) {
  sample::ugrpc::StreamGreetingRequest request;
  sample::ugrpc::StreamGreetingResponse response;
  auto context = std::make_unique<grpc::ClientContext>();

  auto call = Client().Chat(std::move(context));

  WaitClientDeadline();
  bool res{false};
  // I don't really like that in other methods exception is thrown, but here we
  // have False as a result
  UEXPECT_NO_THROW(res = call.Write(request));
  EXPECT_FALSE(res);
}

UTEST_F(GrpcDeadlinePropagation, TestClientChatRead) {
  std::vector<sample::ugrpc::StreamGreetingRequest> requests(3);
  sample::ugrpc::StreamGreetingResponse response;
  auto context = std::make_unique<grpc::ClientContext>();

  auto call = Client().Chat(std::move(context));

  bool res = false;

  for (size_t i = 0; i < requests.size(); ++i) {
    CheckSuccessWrite(call, requests[i], kRequests[i]);
  }

  ASSERT_TRUE(call.WritesDone());

  CheckSuccessRead(call, response, kResponses[0]);
  CheckSuccessRead(call, response, kResponses[1]);

  UEXPECT_THROW(res = call.Read(response),
                ugrpc::client::DeadlineExceededError);
}

UTEST_F(GrpcDeadlinePropagation, TestClientChatReadAsync) {
  std::vector<sample::ugrpc::StreamGreetingRequest> requests(3);
  sample::ugrpc::StreamGreetingResponse response;
  auto context = std::make_unique<grpc::ClientContext>();

  auto call = Client().Chat(std::move(context));

  for (size_t i = 0; i < requests.size(); ++i) {
    CheckSuccessWrite(call, requests[i], kRequests[i]);
  }

  ASSERT_TRUE(call.WritesDone());

  CheckSuccessRead(call, response, kResponses[0]);
  CheckSuccessRead(call, response, kResponses[1]);

  auto future = call.ReadAsync(response);
  UEXPECT_THROW(future.Get(), ugrpc::client::DeadlineExceededError);
}

namespace {

class UnitTestInheritedDeadline final
    : public sample::ugrpc::UnitTestServiceBase {
 public:
  void SayHello(SayHelloCall& call,
                sample::ugrpc::GreetingRequest&& request) override {
    const auto& inherited_data = server::request::kTaskInheritedData.Get();

    EXPECT_TRUE(inherited_data.deadline.IsReachable());
    EXPECT_EQ(inherited_data.path, "sample.ugrpc.UnitTestService");
    EXPECT_EQ(inherited_data.method, "SayHello");

    EXPECT_GT(initial_client_timeout_, engine::Deadline::Duration{})
        << "Not initialized";

    const auto inherited_time_left = inherited_data.deadline.TimeLeft();
    EXPECT_GT(initial_client_timeout_, inherited_time_left)
        << "initial_client_timeout_=" << initial_client_timeout_.count()
        << " vs. inherited_time_left=" << inherited_time_left.count();

    sample::ugrpc::GreetingResponse response;
    response.set_name("Hello " + request.name());

    call.Finish(response);
  }

  void SetClientInitialTimeout(engine::Deadline::Duration value) {
    initial_client_timeout_ = value;
  }

 private:
  engine::Deadline::Duration initial_client_timeout_{};
};

using GrpcTestInheritedDedline =
    ugrpc::tests::ServiceFixture<UnitTestInheritedDeadline>;

}  // namespace

UTEST_F(GrpcTestInheritedDedline, TestServerDataExist) {
  auto client = MakeClient<sample::ugrpc::UnitTestServiceClient>();
  sample::ugrpc::GreetingRequest out;
  out.set_name("userver");

  auto context = std::make_unique<grpc::ClientContext>();
  engine::Deadline deadline =
      engine::Deadline::FromDuration(tests::kLongTimeout);

  GetService().SetClientInitialTimeout(tests::kLongTimeout);
  context->set_deadline(deadline);

  // In tests the gpr_timespec <> steady_clock conversions were giving
  // ~0.5ms precision loss once in 10k runs. Thus the 10ms delay.
  engine::SleepFor(std::chrono::milliseconds{10});

  auto call = client.SayHello(out, std::move(context));

  sample::ugrpc::GreetingResponse in;
  UEXPECT_NO_THROW(in = call.Finish());
  EXPECT_EQ("Hello " + out.name(), in.name());
}

UTEST_F(GrpcTestInheritedDedline, TestDeadlineExpiresBeforeCall) {
  auto client = MakeClient<sample::ugrpc::UnitTestServiceClient>();
  sample::ugrpc::GreetingRequest out;
  out.set_name("userver");

  auto context = std::make_unique<grpc::ClientContext>();
  engine::Deadline deadline =
      engine::Deadline::FromDuration(tests::kShortTimeout);
  context->set_deadline(deadline);

  // Test that the time between client context
  // construction and client request is measured.
  engine::SleepFor(tests::kLongTimeout);

  auto call = client.SayHello(out, std::move(context));

  sample::ugrpc::GreetingResponse in;
  UEXPECT_THROW(in = call.Finish(), ugrpc::client::DeadlineExceededError);
}

namespace {

class UnitTestClientNotSend final : public sample::ugrpc::UnitTestServiceBase {
 public:
  void SayHello(SayHelloCall& /*call*/,
                sample::ugrpc::GreetingRequest&& /*request*/) override {
    FAIL();
  }
};

class GrpcTestClientNotSendData
    : public ugrpc::tests::ServiceFixture<UnitTestClientNotSend> {
 public:
  using ClientType = sample::ugrpc::UnitTestServiceClient;

  GrpcTestClientNotSendData() : client_(MakeClient<ClientType>()) {}

  ClientType& Client() { return client_; }

 private:
  ClientType client_;
};

}  // namespace

UTEST_F(GrpcTestClientNotSendData, TestClientDoNotStartCallWithoutDeadline) {
  auto task_deadline = engine::Deadline::FromDuration(tests::kShortTimeout);
  tests::InitTaskInheritedDeadline(task_deadline);

  sample::ugrpc::GreetingRequest request;
  request.set_name("userver");

  // Wait for deadline before request
  tests::WaitUntilRpcDeadlineClient(task_deadline);
  // Context deadline not set
  auto call = Client().SayHello(request, tests::GetContext(false));

  sample::ugrpc::GreetingResponse in;
  UEXPECT_THROW(in = call.Finish(), ugrpc::client::DeadlineExceededError);
}

UTEST_F(GrpcTestClientNotSendData, TestClientDoNotStartCallWithDeadline) {
  auto task_deadline = engine::Deadline::FromDuration(tests::kShortTimeout);
  tests::InitTaskInheritedDeadline(task_deadline);

  sample::ugrpc::GreetingRequest request;
  request.set_name("userver");

  // Wait for deadline before request
  tests::WaitUntilRpcDeadlineClient(task_deadline);
  // Set additional client deadline
  auto call = Client().SayHello(request, tests::GetContext(true));

  sample::ugrpc::GreetingResponse in;
  UEXPECT_THROW(in = call.Finish(), ugrpc::client::DeadlineExceededError);
}

USERVER_NAMESPACE_END
