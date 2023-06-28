#include <gtest/gtest.h>

#include <chrono>
#include <memory>

#include <grpcpp/client_context.h>
#include <grpcpp/support/time.h>

#include <userver/engine/deadline.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/server/request/task_inherited_data.hpp>
#include <userver/utest/assert_macros.hpp>
#include <userver/utest/utest.hpp>
#include <userver/utils/impl/userver_experiments.hpp>

#include <ugrpc/client/impl/client_configs.hpp>
#include <ugrpc/server/impl/server_configs.hpp>
#include <ugrpc/server/middlewares/deadline_propagation/middleware.hpp>
#include <userver/ugrpc/client/exceptions.hpp>

#include <tests/messages.pb.h>
#include <tests/deadline_tests_helpers.hpp>
#include <tests/service_fixture_test.hpp>
#include <tests/unit_test_client.usrv.pb.hpp>
#include <tests/unit_test_service.usrv.pb.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

using ClientType = sample::ugrpc::UnitTestServiceClient;

constexpr auto kDeadlinePropagated = "deadline-propagated";
constexpr auto kCancelledByDp = "cancelled-by-deadline-propagation";

class UnitTestDeadlineStatsService final
    : public sample::ugrpc::UnitTestServiceBase {
 public:
  void SayHello(SayHelloCall& call,
                sample::ugrpc::GreetingRequest&& request) override {
    sample::ugrpc::GreetingResponse response;
    response.set_name("Hello " + request.name());

    if (wait_deadline_) {
      helpers::WaitForDeadline(call.GetContext().deadline());
    }

    call.Finish(response);
  }

  void SetWaitDeadline(bool value) { wait_deadline_ = value; }

 private:
  bool wait_deadline_{false};
};

class DeadlineStatsTests
    : public GrpcServiceFixtureSimple<UnitTestDeadlineStatsService> {
 public:
  DeadlineStatsTests() {
    ExtendDynamicConfig({
        {ugrpc::client::impl::kEnforceClientTaskDeadline, true},
        {ugrpc::server::impl::kServerCancelTaskByDeadline, true},
    });
    experiments_.Set(utils::impl::kGrpcClientDeadlinePropagationExperiment,
                     true);
    experiments_.Set(utils::impl::kGrpcServerDeadlinePropagationExperiment,
                     true);
    GetServerMiddlewares().push_back(
        std::make_shared<
            ugrpc::server::middlewares::deadline_propagation::Middleware>());
  }

  void BeSlow() { GetService().SetWaitDeadline(true); }

  void BeFast() { GetService().SetWaitDeadline(false); }

  bool ExecuteRequest(bool need_deadline) {
    sample::ugrpc::GreetingRequest request;
    sample::ugrpc::GreetingResponse response;
    request.set_name("abacaba");

    auto client = MakeClient<ClientType>();

    auto context = helpers::GetContext(need_deadline);
    auto call = client.SayHello(request, std::move(context));
    try {
      response = call.Finish();
      EXPECT_EQ(response.name(), "Hello abacaba");
      return true;
    } catch (const ugrpc::client::DeadlineExceededError& /*exception*/) {
      return false;
    }
  }

  void DisableClientDp() {
    ExtendDynamicConfig(
        {{ugrpc::client::impl::kEnforceClientTaskDeadline, false}});
  }

  void ValidateServerStatistic(const std::string& path, size_t expected) {
    const auto statistics = GetStatistics(
        "grpc.server.by-destination",
        {{"grpc_destination", "sample.ugrpc.UnitTestService/SayHello"}});

    EXPECT_EQ(statistics.SingleMetric(path).AsInt(), expected);
  }

  void ValidateClientStatistic(const std::string& path, size_t expected) {
    const auto statistics = GetStatistics(
        "grpc.client.by-destination",
        {{"grpc_destination", "sample.ugrpc.UnitTestService/SayHello"}});

    EXPECT_EQ(statistics.SingleMetric(path).AsInt(), expected);
  }

 private:
  utils::impl::UserverExperimentsScope experiments_;
};

}  // namespace

UTEST_F(DeadlineStatsTests, ServerDeadlineUpdated) {
  constexpr std::size_t kExpected{3};

  // Requests with deadline
  EXPECT_TRUE(ExecuteRequest(true));
  EXPECT_TRUE(ExecuteRequest(true));
  EXPECT_TRUE(ExecuteRequest(true));

  ValidateServerStatistic(kDeadlinePropagated, kExpected);

  // Requests without deadline, default deadline is used
  EXPECT_TRUE(ExecuteRequest(false));
  EXPECT_TRUE(ExecuteRequest(false));
  EXPECT_TRUE(ExecuteRequest(false));

  ValidateServerStatistic(kDeadlinePropagated, kExpected);
}

UTEST_F(DeadlineStatsTests, ClientDeadlineUpdated) {
  size_t expected_value{0};

  // TaskInheritedData has set up. Deadline will be propagated
  helpers::InitTaskInheritedDeadline();

  // Enabled be default
  // Requests with deadline
  // TaskInheritedData less than context deadline and replace it
  EXPECT_TRUE(ExecuteRequest(true));
  EXPECT_TRUE(ExecuteRequest(true));
  EXPECT_TRUE(ExecuteRequest(true));

  expected_value += 3;
  ValidateClientStatistic(kDeadlinePropagated, expected_value);

  // Requests without deadline
  // TaskInheritedData will be set as deadline
  EXPECT_TRUE(ExecuteRequest(false));
  EXPECT_TRUE(ExecuteRequest(false));
  EXPECT_TRUE(ExecuteRequest(false));

  expected_value += 3;
  ValidateClientStatistic(kDeadlinePropagated, expected_value);
}

UTEST_F(DeadlineStatsTests, ClientDeadlineNotUpdated) {
  constexpr std::size_t kExpected{0};

  // TaskInheritedData has set up but context deadline is less
  helpers::InitTaskInheritedDeadline(
      engine::Deadline::FromDuration(helpers::kLongTimeout * 2));

  // Requests with deadline. Deadline will not be replaced
  EXPECT_TRUE(ExecuteRequest(true));
  EXPECT_TRUE(ExecuteRequest(true));
  EXPECT_TRUE(ExecuteRequest(true));

  ValidateClientStatistic(kDeadlinePropagated, kExpected);

  // Disable deadline propagation for the following tests
  const server::request::DeadlinePropagationBlocker dp_blocker;

  // Requests with deadline
  EXPECT_TRUE(ExecuteRequest(true));
  EXPECT_TRUE(ExecuteRequest(true));
  EXPECT_TRUE(ExecuteRequest(true));

  ValidateClientStatistic(kDeadlinePropagated, kExpected);

  // Requests without deadline
  EXPECT_TRUE(ExecuteRequest(false));
  EXPECT_TRUE(ExecuteRequest(false));
  EXPECT_TRUE(ExecuteRequest(false));

  ValidateClientStatistic(kDeadlinePropagated, kExpected);
}

UTEST_F(DeadlineStatsTests, ClientDeadlineCancelled) {
  constexpr std::size_t kExpected{1};

  // Server will wait for deadline before answer
  BeSlow();

  // TaskInheritedData has set up, but DP disabled
  helpers::InitTaskInheritedDeadline();

  // Requests with deadline
  EXPECT_FALSE(ExecuteRequest(true));

  ValidateClientStatistic(kCancelledByDp, kExpected);
}

UTEST_F(DeadlineStatsTests, ClientDeadlineCancelledNotByDp) {
  constexpr std::size_t kExpected{0};

  // Server will wait for deadline before answer
  BeSlow();

  // No TaskInheritedData. Deadline not updated. Request cancelled but not
  // due to deadline propagation

  // Requests with deadline
  EXPECT_FALSE(ExecuteRequest(true));

  ValidateClientStatistic(kCancelledByDp, kExpected);
}

UTEST_F(DeadlineStatsTests, DisabledClientDeadlineUpdated) {
  constexpr std::size_t kExpected{0};

  DisableClientDp();

  // TaskInheritedData has set up, but DP disabled
  helpers::InitTaskInheritedDeadline();

  // Requests with deadline
  // TaskInheritedData ignored
  EXPECT_TRUE(ExecuteRequest(true));
  EXPECT_TRUE(ExecuteRequest(true));
  EXPECT_TRUE(ExecuteRequest(true));

  // Requests without deadline
  // TaskInheritedData ignored
  EXPECT_TRUE(ExecuteRequest(false));
  EXPECT_TRUE(ExecuteRequest(false));
  EXPECT_TRUE(ExecuteRequest(false));

  ValidateClientStatistic(kDeadlinePropagated, kExpected);
}

UTEST_F(DeadlineStatsTests, DisabledClientDeadlineCancelled) {
  constexpr std::size_t kExpected{0};

  BeSlow();

  DisableClientDp();

  // TaskInheritedData has set up, but DP disabled.
  helpers::InitTaskInheritedDeadline();

  // Failed by deadline. But not due to deadline propagation
  EXPECT_FALSE(ExecuteRequest(true));

  ValidateClientStatistic(kCancelledByDp, kExpected);
}

USERVER_NAMESPACE_END
