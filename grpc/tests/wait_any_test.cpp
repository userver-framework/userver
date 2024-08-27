#include <userver/utest/utest.hpp>

#include <grpcpp/grpcpp.h>

#include <ugrpc/impl/status.hpp>
#include <userver/engine/condition_variable.hpp>
#include <userver/engine/deadline.hpp>
#include <userver/engine/mutex.hpp>
#include <userver/engine/single_consumer_event.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/engine/wait_any.hpp>
#include <userver/ugrpc/client/exceptions.hpp>
#include <userver/utils/async.hpp>
#include <userver/utils/from_string.hpp>
#include <userver/utils/text.hpp>

#include <tests/unit_test_client.usrv.pb.hpp>
#include <tests/unit_test_service.usrv.pb.hpp>
#include <userver/ugrpc/tests/service_fixtures.hpp>

using namespace std::chrono_literals;

USERVER_NAMESPACE_BEGIN

namespace {

class AsyncTestService final : public sample::ugrpc::UnitTestServiceBase {
 public:
  void SayHello(SayHelloCall& call, sample::ugrpc::GreetingRequest&&) override {
    // Only send response on manual triggers
    bool wait_result = false;
    {
      std::unique_lock ulock{mutex_};
      wait_result = cv_.Wait(ulock, [this]() { return answers_count_ > 0; });
      // here, we have lock
      // drop flag back
      answers_count_--;
      // lock is dropped automatically upon exiting this scope
    }

    if (!wait_result) {
      call.FinishWithError(
          {grpc::StatusCode::INTERNAL, "failed to wait for event", "details"});
      return;
    }
    sample::ugrpc::GreetingResponse response;
    response.set_name("Hello");
    call.Finish(response);
  }

  // This method can be called from the tests
  void TriggerChatResponse() {
    {
      std::lock_guard guard{mutex_};
      answers_count_++;
    }
    cv_.NotifyOne();
  }

 private:
  engine::Mutex mutex_;
  std::size_t answers_count_{0};
  engine::ConditionVariable cv_;
};

}  // namespace

using GrpcClientWaitAnyTest = ugrpc::tests::ServiceFixture<AsyncTestService>;

UTEST_F_MT(GrpcClientWaitAnyTest, HappyPath, 4) {
  auto client = MakeClient<sample::ugrpc::UnitTestServiceClient>();
  // Make two calls
  auto call1 = client.SayHello({});
  auto call2 = client.SayHello({});

  sample::ugrpc::GreetingResponse response1;
  sample::ugrpc::GreetingResponse response2;

  auto future1 = call1.FinishAsync(response1);
  auto future2 = call2.FinishAsync(response2);

  // Launch WaitAny in separate task
  auto wait_task = utils::Async("wait_any", [&]() -> bool {
    auto success_idx_opt = engine::WaitAny(future1, future2);
    // consume future
    if (*success_idx_opt == 0) {
      future1.Get();
    } else {
      future2.Get();
    }

    return success_idx_opt.has_value();
  });

  // Answer exactly one chat request
  GetService().TriggerChatResponse();

  // Now, wait for result
  EXPECT_TRUE(wait_task.Get());

  // send second response, or we won't be able to finish unit-test
  // without exception
  GetService().TriggerChatResponse();
}

UTEST_F_MT(GrpcClientWaitAnyTest, GrcpCallCancelledAtFutureDestruction, 4) {
  auto client = MakeClient<sample::ugrpc::UnitTestServiceClient>();

  // Launch WaitAny in separate task
  auto wait_task = utils::Async("wait_any", [&]() -> bool {
    auto call1 = client.SayHello({});
    auto call2 = client.SayHello({});

    sample::ugrpc::GreetingResponse response1;
    sample::ugrpc::GreetingResponse response2;

    auto future1 = call1.FinishAsync(response1);
    auto future2 = call2.FinishAsync(response2);

    auto success_idx_opt = engine::WaitAny(future1, future2);
    if (*success_idx_opt == 0) {
      future1.Get();
    } else {
      future2.Get();
    }

    return success_idx_opt.has_value();
  });

  // Answer exactly one chat request
  GetService().TriggerChatResponse();

  // Now, wait for result
  EXPECT_TRUE(wait_task.Get());

  // We got response only for one gRPC call, but the second one has been
  // cancelled. So, we don't need to send response for second call.
}

UTEST_F_MT(GrpcClientWaitAnyTest, SingleCancel, 2) {
  auto client = MakeClient<sample::ugrpc::UnitTestServiceClient>();
  // Make two calls
  auto call1 = client.SayHello({});

  sample::ugrpc::GreetingResponse response1;

  auto future1 = call1.FinishAsync(response1);

  engine::SingleConsumerEvent wait_task_started;

  // Launch WaitAny in separate task
  auto wait_task = utils::Async("wait_any", [&]() -> bool {
    // notify that we have started
    wait_task_started.Send();
    auto success_idx_opt = engine::WaitAny(future1);
    // In this test we should exit by canceling coroutine itself
    // so success_idx_opt should be nullopt
    EXPECT_FALSE(success_idx_opt.has_value());
    return success_idx_opt.has_value();
  });

  ASSERT_TRUE(wait_task_started.WaitForEvent());

  // This is not enough. We want to make sure that wait_task has actually
  // entered 'waiting' part
  while (wait_task.GetState() != engine::TaskBase::State::kSuspended) {
    engine::InterruptibleSleepFor(std::chrono::milliseconds{10});
  }

  // cancel coroutine
  wait_task.SyncCancel();

  // send response, or we won't be able to finish unit-test
  // without exception
  GetService().TriggerChatResponse();
}

UTEST_F_MT(GrpcClientWaitAnyTest, FutureDestructionAtCancel, 2) {
  auto client = MakeClient<sample::ugrpc::UnitTestServiceClient>();

  engine::SingleConsumerEvent wait_task_started;

  // Launch WaitAny in separate task
  auto wait_task =
      utils::Async("wait_any", [&]() -> std::optional<std::size_t> {
        auto call1 = client.SayHello({});
        sample::ugrpc::GreetingResponse response1;
        auto future1 = call1.FinishAsync(response1);

        // notify that we have started
        wait_task_started.Send();
        auto success_idx_opt = engine::WaitAny(future1);
        return success_idx_opt;
      });

  ASSERT_TRUE(wait_task_started.WaitForEvent());

  // This is not enough. We want to make sure that wait_task has actually
  // entered 'waiting' part
  while (wait_task.GetState() != engine::TaskBase::State::kSuspended) {
    engine::InterruptibleSleepFor(std::chrono::milliseconds{10});
  }

  // cancel coroutine
  wait_task.SyncCancel();

  // In this test we should exit by canceling coroutine itself
  // so wait_task result should be nullopt
  EXPECT_EQ(wait_task.Get(), std::nullopt);

  // Note that we don't need answer from server now, as the call already
  // cancelled
}

UTEST_F_MT(GrpcClientWaitAnyTest, DoubleCall, 2) {
  // In this test we check that calling WaitAny on future twice, without
  // calling Get() doesn't lead to segfault

  auto client = MakeClient<sample::ugrpc::UnitTestServiceClient>();
  auto call1 = client.SayHello({});

  sample::ugrpc::GreetingResponse response1;

  auto future1 = call1.FinishAsync(response1);

  // Launch WaitAny in separate task
  auto wait_task = utils::Async("wait_any", [&]() -> void {
    // notify that we have started
    auto success_idx_opt = engine::WaitAny(future1);
    EXPECT_TRUE(success_idx_opt.has_value());
    // call again, without consuming future
    auto success_idx_opt2 = engine::WaitAny(future1);
    ASSERT_EQ(success_idx_opt, success_idx_opt2);
    return;
  });

  // send response
  GetService().TriggerChatResponse();

  // wait for finish
  wait_task.Get();
}

UTEST_F_MT(GrpcClientWaitAnyTest, CallAfterGet, 2) {
  // In this test we check that calling WaitAny on future after we have called
  // Get on it doesn't lead to segfault

  auto client = MakeClient<sample::ugrpc::UnitTestServiceClient>();
  auto call1 = client.SayHello({});

  sample::ugrpc::GreetingResponse response1;

  auto future1 = call1.FinishAsync(response1);

  // Launch WaitAny in separate task
  auto wait_task = utils::Async("wait_any", [&]() -> void {
    // notify that we have started
    auto success_idx_opt = engine::WaitAny(future1);
    ASSERT_TRUE(success_idx_opt.has_value());
    future1.Get();  // consume future
    // call again
    auto success_idx_opt2 = engine::WaitAny(future1);
    ASSERT_FALSE(success_idx_opt2.has_value());
    return;
  });

  // send response
  GetService().TriggerChatResponse();

  // wait for finish
  wait_task.Get();
}

UTEST_F_MT(GrpcClientWaitAnyTest, WaitInLoop, 2) {
  // In this test we check common pattern - make requests, put them in array
  // and wait one-by-one until completion
  auto client = MakeClient<sample::ugrpc::UnitTestServiceClient>();
  // Make two calls
  auto call1 = client.SayHello({});
  auto call2 = client.SayHello({});
  auto call3 = client.SayHello({});
  auto call4 = client.SayHello({});

  sample::ugrpc::GreetingResponse response1;
  sample::ugrpc::GreetingResponse response2;
  sample::ugrpc::GreetingResponse response3;
  sample::ugrpc::GreetingResponse response4;

  std::vector<ugrpc::client::UnaryFuture> futures;
  futures.emplace_back(call1.FinishAsync(response1));
  futures.emplace_back(call2.FinishAsync(response2));
  futures.emplace_back(call3.FinishAsync(response3));
  futures.emplace_back(call4.FinishAsync(response4));

  utils::FixedArray<bool> got_responses_flag{futures.size(), false};

  ASSERT_EQ(futures.size(), got_responses_flag.size());

  engine::SingleConsumerEvent wait_task_started;

  // Launch WaitAny in separate task
  auto wait_task = utils::Async("wait_any", [&]() -> void {
    wait_task_started.Send();
    for (std::size_t i = 0; i < futures.size(); ++i) {
      auto success_idx_opt = engine::WaitAny(futures);
      // check that we have the response
      ASSERT_TRUE(success_idx_opt.has_value());
      // check that this is new future, not some previous one
      ASSERT_LT(*success_idx_opt, futures.size());  // sanity check
      ASSERT_EQ(got_responses_flag[*success_idx_opt], false);
      got_responses_flag[*success_idx_opt] = true;
      // consume future
      futures[*success_idx_opt].Get();
    }
    return;
  });

  ASSERT_TRUE(wait_task_started.WaitForEvent());
  // This is not enough. We want to make sure that wait_task has actually
  // entered 'waiting' part
  while (wait_task.GetState() != engine::TaskBase::State::kSuspended) {
    engine::InterruptibleSleepFor(std::chrono::milliseconds{10});
  }

  // send responses
  for (std::size_t i = 0; i < futures.size(); ++i) {
    GetService().TriggerChatResponse();
  }

  wait_task.Get();
}

UTEST_F_MT(GrpcClientWaitAnyTest, ServerTimeout, 2) {
  // In this test we check that calling with low deadline and not receiving
  // response from server will not cause segfault

  auto client = MakeClient<sample::ugrpc::UnitTestServiceClient>();
  auto context = std::make_unique<grpc::ClientContext>();

  std::chrono::milliseconds deadline_ms{1500};
  auto deadline = engine::Deadline::FromDuration(deadline_ms);
  context->set_deadline(deadline);

  auto call1 = client.SayHello({}, std::move(context));

  sample::ugrpc::GreetingResponse response1;

  auto future1 = call1.FinishAsync(response1);

  // Launch WaitAny in separate task
  auto wait_task = utils::Async("wait_any", [&]() -> void {
    // notify that we have started
    auto success_idx_opt = engine::WaitAny(future1);
    // We kinda receive response 'from server' - grpc core will give us
    // DEADLINE_EXCEEDED
    EXPECT_TRUE(success_idx_opt.has_value());
    return;
  });

  // First, wait for wait_task. This will ensure that server sends no
  // response
  wait_task.Get();

  // Then send response, otherwise we crash when exiting test, because there
  // is a coroutine that waits for 'NotifyOne', but we are destroying the
  // whole object.
  GetService().TriggerChatResponse();
}

USERVER_NAMESPACE_END
