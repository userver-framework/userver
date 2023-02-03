#include <userver/utest/utest.hpp>

#include <google/rpc/error_details.pb.h>
#include <google/rpc/status.pb.h>

#include <ugrpc/impl/status.hpp>
#include <userver/engine/deadline.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/ugrpc/client/exceptions.hpp>

#include <tests/service_fixture_test.hpp>
#include <tests/unit_test_client.usrv.pb.hpp>
#include <tests/unit_test_service.usrv.pb.hpp>

using namespace std::chrono_literals;

USERVER_NAMESPACE_BEGIN

namespace {

constexpr int kNumber = 42;

class AsyncTestServiceWithError final
    : public sample::ugrpc::UnitTestServiceBase {
 public:
  void Chat(ChatCall& call) override {
    call.FinishWithError({grpc::StatusCode::INTERNAL, "message", "details"});
  }
};

class AsyncTestService final : public sample::ugrpc::UnitTestServiceBase {
 public:
  void Chat(ChatCall& call) override {
    sample::ugrpc::StreamGreetingResponse response;
    response.set_number(kNumber);
    response.set_name("Hello");
    call.WriteAndFinish(response);
  }
};

}  // namespace

using GrpcAsyncClientErrorTest =
    GrpcServiceFixtureSimple<AsyncTestServiceWithError>;
using GrpcAsyncClientTest = GrpcServiceFixtureSimple<AsyncTestService>;

// Disabled due to https://github.com/grpc/grpc/issues/14812
UTEST_F(GrpcAsyncClientErrorTest, DISABLED_BidirectionalStreamAsyncRead) {
  auto client = MakeClient<sample::ugrpc::UnitTestServiceClient>();
  sample::ugrpc::StreamGreetingResponse in;
  sample::ugrpc::StreamGreetingRequest out;
  auto call = client.Chat();
  // This future will never complete with a response, because the service writes
  // nothing
  auto future = call.ReadAsync(in);

  out.set_name("userver");
  out.set_number(42);
  // NOLINTNEXTLINE(clang-analyzer-optin.cplusplus.UninitializedObject)
  EXPECT_TRUE(call.Write(out));

  auto write_result = true;
  const auto deadline = engine::Deadline::FromDuration(utest::kMaxTestWaitTime);
  while (!deadline.IsReached()) {
    out.set_name("write_fail");
    out.set_number(0xDEAD);
    // NOLINTNEXTLINE(clang-analyzer-optin.cplusplus.UninitializedObject);
    write_result = call.Write(out);
    if (!write_result) {
      break;
    }
    engine::Yield();
  }

  EXPECT_FALSE(write_result);
  // NOLINTNEXTLINE(clang-analyzer-optin.cplusplus.UninitializedObject)
  UEXPECT_THROW(static_cast<void>(future.Get()), ugrpc::client::InternalError);
}

UTEST_F(GrpcAsyncClientTest, BidirectionalStreamAsyncRead) {
  auto client = MakeClient<sample::ugrpc::UnitTestServiceClient>();
  auto is = client.Chat();

  sample::ugrpc::StreamGreetingResponse in;
  auto future_for_move = is.ReadAsync(in);
  auto future = std::move(future_for_move);

  bool result = false;
  UEXPECT_NO_THROW((result = future.Get()));
  EXPECT_TRUE(result);
  EXPECT_EQ(in.number(), kNumber);
  EXPECT_EQ(in.name(), "Hello");

  auto future_last_read = is.ReadAsync(in);

  bool is_ready = false;
  UEXPECT_NO_THROW((is_ready = future_last_read.IsReady()));
  UEXPECT_NO_THROW((result = future_last_read.Get()));
  EXPECT_FALSE(result);
}

USERVER_NAMESPACE_END
