#include <gtest/gtest.h>

#include <atomic>

#include <userver/engine/async.hpp>
#include <userver/engine/single_consumer_event.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/logging/log.hpp>

#include <userver/utest/utest.hpp>

USERVER_NAMESPACE_BEGIN

TEST(SingleConsumerEvent, Ctr) {
  engine::SingleConsumerEvent event;
  EXPECT_TRUE(event.IsAutoReset());
}

UTEST(SingleConsumerEvent, WaitAndCancel) {
  engine::SingleConsumerEvent event;
  auto task =
      engine::AsyncNoSpan([&event]() { EXPECT_FALSE(event.WaitForEvent()); });

  task.WaitFor(std::chrono::milliseconds(50));
  EXPECT_FALSE(task.IsFinished());
}

UTEST(SingleConsumerEvent, WaitAndSend) {
  engine::SingleConsumerEvent event;
  auto task =
      engine::AsyncNoSpan([&event]() { EXPECT_TRUE(event.WaitForEvent()); });

  engine::SleepFor(std::chrono::milliseconds(50));
  event.Send();

  task.WaitFor(std::chrono::milliseconds(50));
  EXPECT_TRUE(task.IsFinished());
}

UTEST(SingleConsumerEvent, WaitAndSendDouble) {
  engine::SingleConsumerEvent event;
  auto task = engine::AsyncNoSpan([&event]() {
    for (int i = 0; i < 2; i++) EXPECT_TRUE(event.WaitForEvent());
  });

  for (int i = 0; i < 2; i++) {
    engine::SleepFor(std::chrono::milliseconds(50));
    event.Send();
  }

  task.WaitFor(std::chrono::milliseconds(50));
  EXPECT_TRUE(task.IsFinished());
}

UTEST(SingleConsumerEvent, SendAndWait) {
  engine::SingleConsumerEvent event;
  std::atomic<bool> is_event_sent{false};

  auto task = engine::AsyncNoSpan([&event, &is_event_sent]() {
    while (!is_event_sent) engine::SleepFor(std::chrono::milliseconds(10));
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

  engine::SleepFor(std::chrono::milliseconds(10));
  for (size_t i = 0; i < count; i++) {
    event.Send();
  }
  engine::SleepFor(std::chrono::milliseconds(10));

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

USERVER_NAMESPACE_END
