#include <userver/engine/single_consumer_event.hpp>

#include <atomic>

#include <userver/engine/async.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/logging/log.hpp>
#include <userver/utest/utest.hpp>

using namespace std::chrono_literals;

USERVER_NAMESPACE_BEGIN

TEST(SingleConsumerEvent, Ctr) {
  engine::SingleConsumerEvent event;
  EXPECT_TRUE(event.IsAutoReset());
  EXPECT_FALSE(event.IsReady());
}

UTEST(SingleConsumerEvent, IsReady) {
  engine::SingleConsumerEvent event;
  event.Send();
  EXPECT_TRUE(event.IsReady());
  EXPECT_TRUE(event.IsReady());

  EXPECT_TRUE(event.WaitForEvent());
  EXPECT_FALSE(event.IsReady());
}

UTEST(SingleConsumerEvent, WaitAndCancel) {
  engine::SingleConsumerEvent event;
  auto task =
      engine::AsyncNoSpan([&event]() { EXPECT_FALSE(event.WaitForEvent()); });

  task.WaitFor(50ms);
  EXPECT_FALSE(task.IsFinished());
}

UTEST(SingleConsumerEvent, WaitAndSend) {
  engine::SingleConsumerEvent event;
  auto task =
      engine::AsyncNoSpan([&event]() { EXPECT_TRUE(event.WaitForEvent()); });

  engine::SleepFor(50ms);
  event.Send();

  task.WaitFor(50ms);
  EXPECT_TRUE(task.IsFinished());
}

UTEST(SingleConsumerEvent, WaitAndSendDouble) {
  engine::SingleConsumerEvent event;
  auto task = engine::AsyncNoSpan([&event]() {
    for (int i = 0; i < 2; i++) EXPECT_TRUE(event.WaitForEvent());
  });

  for (int i = 0; i < 2; i++) {
    engine::SleepFor(50ms);
    event.Send();
  }

  task.WaitFor(50ms);
  EXPECT_TRUE(task.IsFinished());
}

UTEST(SingleConsumerEvent, SendAndWait) {
  engine::SingleConsumerEvent event;
  std::atomic<bool> is_event_sent{false};

  auto task = engine::AsyncNoSpan([&event, &is_event_sent]() {
    while (!is_event_sent) engine::SleepFor(10ms);
    EXPECT_TRUE(event.WaitForEvent());
  });

  event.Send();
  is_event_sent = true;

  task.WaitFor(utest::kMaxTestWaitTime);
  EXPECT_TRUE(task.IsFinished());
}

UTEST(SingleConsumerEvent, WaitFailed) {
  engine::SingleConsumerEvent event;

  EXPECT_FALSE(event.WaitForEventUntil(engine::Deadline::Passed()));
}

UTEST(SingleConsumerEvent, SendAndWait2) {
  engine::SingleConsumerEvent event;
  auto task = engine::AsyncNoSpan([&event]() {
    EXPECT_TRUE(event.WaitForEvent());
    EXPECT_TRUE(event.WaitForEvent());
  });

  event.Send();
  engine::Yield();
  event.Send();
  engine::Yield();

  EXPECT_TRUE(task.IsFinished());
}

UTEST(SingleConsumerEvent, SendAndWait3) {
  engine::SingleConsumerEvent event;
  auto task = engine::AsyncNoSpan([&event]() {
    EXPECT_TRUE(event.WaitForEvent());
    EXPECT_TRUE(event.WaitForEvent());
    EXPECT_FALSE(event.WaitForEvent());
  });

  event.Send();
  engine::Yield();
  event.Send();
  engine::Yield();
}

UTEST_MT(SingleConsumerEvent, Multithread, 2) {
  const auto count = 10000;

  engine::SingleConsumerEvent event;
  std::atomic<int> got{0};

  auto task = engine::AsyncNoSpan([&got, &event]() {
    while (event.WaitForEvent()) {
      got++;
    }
  });

  engine::SleepFor(10ms);
  for (size_t i = 0; i < count; i++) {
    event.Send();
  }
  engine::SleepFor(10ms);

  EXPECT_GE(got.load(), 1);
  EXPECT_LE(got.load(), count);
  LOG_INFO() << "waiting";
  task.SyncCancel();
  LOG_INFO() << "waited";
}

UTEST(SingleConsumerEvent, PassBetweenTasks) {
  constexpr size_t kIterations = 4;

  engine::SingleConsumerEvent task_started;
  engine::SingleConsumerEvent event;

  for (size_t i = 0; i < kIterations; ++i) {
    auto task = engine::AsyncNoSpan([&event, &task_started] {
      task_started.Send();
      EXPECT_TRUE(event.WaitForEventFor(utest::kMaxTestWaitTime));
    });
    ASSERT_TRUE(task_started.WaitForEventFor(utest::kMaxTestWaitTime));
    event.Send();
    task.WaitFor(utest::kMaxTestWaitTime);
    EXPECT_TRUE(task.IsFinished());
    UEXPECT_NO_THROW(task.Get());
  }
}

UTEST(SingleConsumerEvent, NoAutoReset) {
  static constexpr auto kNoWait = std::chrono::seconds::zero();

  engine::SingleConsumerEvent event(engine::SingleConsumerEvent::NoAutoReset{});

  EXPECT_FALSE(event.IsAutoReset());
  EXPECT_FALSE(event.WaitForEventFor(kNoWait));

  event.Send();
  EXPECT_TRUE(event.WaitForEventFor(kNoWait));
  EXPECT_TRUE(event.WaitForEventFor(kNoWait));
  event.Reset();
  EXPECT_FALSE(event.WaitForEventFor(kNoWait));
  event.Send();
  EXPECT_TRUE(event.WaitForEventFor(kNoWait));
  EXPECT_TRUE(event.WaitForEventFor(kNoWait));
}

UTEST_MT(SingleConsumerEvent, NoSignalDuplication, 2) {
  engine::SingleConsumerEvent event;
  std::atomic<std::size_t> events_received{0};

  auto waiter = engine::AsyncNoSpan([&] {
    if (event.WaitForEvent()) {
      ++events_received;
    }
    if (event.WaitForEvent()) {
      ++events_received;
    }
  });

  event.Send();

  // Allow 'WaitForEvent' to race with 'Send' for a little while
  engine::SleepFor(10us);

  waiter.SyncCancel();
  ASSERT_LE(events_received, 1);
}

UTEST_MT(SingleConsumerEvent, ParallelSend, 3) {
  constexpr std::size_t kProducersCount = 2;

  const auto test_deadline = engine::Deadline::FromDuration(50ms);
  engine::SingleConsumerEvent event;

  std::vector<engine::TaskWithResult<void>> producers;
  for (std::size_t i = 0; i < kProducersCount; ++i) {
    producers.push_back(engine::CriticalAsyncNoSpan([&] {
      while (!engine::current_task::ShouldCancel()) {
        event.Send();
        engine::Yield();
      }
    }));
  }

  while (!test_deadline.IsReached()) {
    const bool success = event.WaitForEvent();
    ASSERT_TRUE(success);
  }

  for (auto& producer : producers) {
    producer.RequestCancel();
    UEXPECT_NO_THROW(producer.Get());
  }
}

namespace {

auto WaitAndDestroySample() {
  /// [Wait and destroy]
  engine::TaskWithResult<void> sender;
  {
    engine::SingleConsumerEvent event;
    sender = engine::AsyncNoSpan([&event] { event.Send(); });
    // will be woken up by 'Send()' above
    const bool success = event.WaitForEvent();

    if (!success) {
      // If the waiting failed due to deadline or cancellation, we must somehow
      // wait until the parallel task does Send (or prevent the parallel task
      // from ever doing Send) before destroying the SingleConsumerEvent.
      sender.SyncCancel();
    }

    // 'event' is destroyed here. Note that 'Send' might continue executing, but
    // it will still complete safely.
  }
  /// [Wait and destroy]

  // The test succeeds if the sanitizers are happy, and we haven't violated
  // event's lifetime.
  return sender;
}

}  // namespace

UTEST(SingleConsumerEvent, WaitAndDestroySuccess) {
  auto sender = WaitAndDestroySample();
  UEXPECT_NO_THROW(sender.Get());
}

UTEST(SingleConsumerEvent, WaitAndDestroyCancellation) {
  engine::current_task::GetCancellationToken().RequestCancel();

  auto sender = WaitAndDestroySample();

  const engine::TaskCancellationBlocker cancel_blocker;
  UEXPECT_THROW_MSG(sender.Get(), engine::TaskCancelledException,
                    "User request");
}

USERVER_NAMESPACE_END
