#include <utest/utest.hpp>

#include <engine/single_consumer_event.hpp>
#include <engine/sleep.hpp>
#include <engine/task/cancel.hpp>
#include <utils/background_task_storage.hpp>

TEST(BackgroundTasksStorage, TaskStart) {
  RunInCoro([] {
    utils::BackgroundTasksStorage bts;

    engine::SingleConsumerEvent event;
    bts.AsyncDetach("test", [&event] { event.Send(); });

    EXPECT_TRUE(event.WaitForEvent());
  });
}

TEST(BackgroundTasksStorage, CancelAndWaitInDtr) {
  RunInCoro([] {
    std::atomic<bool> started{false}, cancelled{false};
    auto shared = std::make_shared<int>(1);
    {
      utils::BackgroundTasksStorage bts;

      bts.AsyncDetach("test", [shared, &started, &cancelled] {
        started = true;

        engine::SingleConsumerEvent event;
        cancelled = !event.WaitForEventFor(std::chrono::seconds(2));
      });

      engine::SingleConsumerEvent event;
      EXPECT_FALSE(event.WaitForEventFor(std::chrono::milliseconds(50)));

      EXPECT_EQ(shared.use_count(), 2);
    }

    EXPECT_TRUE(started);
    EXPECT_TRUE(cancelled);
    EXPECT_EQ(shared.use_count(), 1);
  });
}
