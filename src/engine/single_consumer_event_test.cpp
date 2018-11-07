#include <gtest/gtest.h>

#include <engine/async.hpp>
#include <engine/single_consumer_event.hpp>
#include <engine/sleep.hpp>
#include <engine/task/task_context.hpp>
#include <logging/log.hpp>

#include <utest/utest.hpp>

TEST(SingleConsumerEvent, WaitListLightLockfree) {
  std::atomic<engine::impl::TaskContext*> wait_list_waiting;
  EXPECT_TRUE(wait_list_waiting.is_lock_free());
}

TEST(SingleConsumerEvent, Ctr) { engine::SingleConsumerEvent event; }

TEST(SingleConsumerEvent, WaitAndCancel) {
  RunInCoro([] {
    engine::SingleConsumerEvent event;
    auto task = engine::Async([&event]() { event.WaitForEvent(); });

    task.WaitFor(std::chrono::milliseconds(50));
    EXPECT_FALSE(task.IsFinished());
  });
}

TEST(SingleConsumerEvent, WaitAndSend) {
  RunInCoro([] {
    engine::SingleConsumerEvent event;
    auto task = engine::Async([&event]() { event.WaitForEvent(); });

    engine::SleepFor(std::chrono::milliseconds(50));
    event.Send();

    task.WaitFor(std::chrono::milliseconds(50));
    EXPECT_TRUE(task.IsFinished());
  });
}

TEST(SingleConsumerEvent, WaitAndSendDouble) {
  RunInCoro([] {
    engine::SingleConsumerEvent event;
    auto task = engine::Async([&event]() {
      for (int i = 0; i < 2; i++) event.WaitForEvent();
    });

    for (int i = 0; i < 2; i++) {
      engine::SleepFor(std::chrono::milliseconds(50));
      event.Send();
    }

    task.WaitFor(std::chrono::milliseconds(50));
    EXPECT_TRUE(task.IsFinished());
  });
}

TEST(SingleConsumerEvent, SendAndWait) {
  RunInCoro([] {
    engine::SingleConsumerEvent event;
    auto task = engine::Async([&event]() {
      engine::SleepFor(std::chrono::milliseconds(50));
      event.WaitForEvent();
    });

    event.Send();

    task.WaitFor(std::chrono::milliseconds(100));
    EXPECT_TRUE(task.IsFinished());
  });
}

TEST(SingleConsumerEvent, SendAndWait2) {
  RunInCoro([] {
    engine::SingleConsumerEvent event;
    auto task = engine::Async([&event]() {
      event.WaitForEvent();
      event.WaitForEvent();
    });

    event.Send();
    engine::Yield();
    event.Send();
    engine::Yield();

    EXPECT_TRUE(task.IsFinished());
  });
}

TEST(SingleConsumerEvent, SendAndWait3) {
  RunInCoro([] {
    engine::SingleConsumerEvent event;
    auto task = engine::Async([&event]() {
      event.WaitForEvent();
      event.WaitForEvent();
      event.WaitForEvent();
    });

    event.Send();
    engine::Yield();
    event.Send();
    engine::Yield();
  });
}

TEST(SingleConsumerEvent, Multithread) {
  const auto threads = 2;
  const auto count = 10000;

  RunInCoro(
      [count] {
        engine::SingleConsumerEvent event;
        std::atomic<int> got{0};

        auto task = engine::Async([&got, &event]() {
          while (true) {
            event.WaitForEvent();
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
        task.RequestCancel();
        task.Wait();
        LOG_INFO() << "waited";
      },
      threads);
}
