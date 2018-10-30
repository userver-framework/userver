#include <gtest/gtest.h>

#include <engine/async.hpp>
#include <engine/single_consumer_event.hpp>
#include <engine/sleep.hpp>

#include <utest/utest.hpp>

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

TEST(SingleConsumerEvent, Multithread) {
  const auto threads = 2;
  const auto count = 100000;

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
        for (size_t i = 0; i < count; i++) event.Send();
        engine::SleepFor(std::chrono::milliseconds(10));

        EXPECT_GE(got.load(), 1);
        EXPECT_LE(got.load(), count);
      },
      threads);
}
