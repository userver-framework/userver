#include <userver/utest/utest.hpp>

#include <userver/engine/single_consumer_event.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/utils/background_task_storage.hpp>

USERVER_NAMESPACE_BEGIN

UTEST(BackgroundTasksStorage, TaskStart) {
  utils::BackgroundTasksStorage bts;

  engine::SingleConsumerEvent event;
  bts.AsyncDetach("test", [&event] { event.Send(); });

  EXPECT_TRUE(event.WaitForEvent());
}

UTEST(BackgroundTasksStorage, CancelAndWaitInDtr) {
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
}

UTEST(BackgroundTasksStorage, NoDeadlockWithUnstartedTasks) {
  utils::BackgroundTasksStorage bts;
  bts.AsyncDetach("test", [] {
    engine::SingleConsumerEvent event;

    EXPECT_FALSE(event.WaitForEvent());
  });
}

UTEST(BackgroundTasksStorage, MutableLambda) {
  utils::BackgroundTasksStorage bts;
  bts.AsyncDetach("test", [value = 1]() mutable {
    ++value;
    return value;
  });
}

UTEST(BackgroundTasksStorage, ActiveTasksCounter) {
  const long kNoopTasks = 2;
  const long kLongTasks = 3;
  utils::BackgroundTasksStorage bts;

  for (int i = 0; i < kNoopTasks; ++i) {
    bts.AsyncDetach("noop-task", [] { /* noop */ });
  }
  for (int i = 0; i < kLongTasks; ++i) {
    bts.AsyncDetach("long-task", [] {
      engine::SingleConsumerEvent event;

      EXPECT_FALSE(event.WaitForEventFor(std::chrono::seconds(2)));
    });
  }

  EXPECT_EQ(bts.ActiveTasksApprox(), kNoopTasks + kLongTasks);

  engine::SingleConsumerEvent event;
  EXPECT_FALSE(event.WaitForEventFor(std::chrono::milliseconds(50)));

  EXPECT_EQ(bts.ActiveTasksApprox(), kLongTasks);
}

USERVER_NAMESPACE_END
