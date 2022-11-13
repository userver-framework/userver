#include <userver/utest/utest.hpp>

#include <atomic>

#include <userver/concurrent/background_task_storage.hpp>
#include <userver/concurrent/background_task_storage_fwd.hpp>
#include <userver/engine/single_consumer_event.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/engine/task/inherited_variable.hpp>
#include <userver/engine/task/task_processor_utils.hpp>
#include <userver/utils/lazy_prvalue.hpp>

using namespace std::chrono_literals;

USERVER_NAMESPACE_BEGIN

UTEST(BackgroundTaskStorage, TaskStart) {
  concurrent::BackgroundTaskStorage bts;

  engine::SingleConsumerEvent event;
  bts.AsyncDetach("test", [&event] { event.Send(); });

  EXPECT_TRUE(event.WaitForEvent());
}

UTEST(BackgroundTaskStorage, CancelAndWaitInDtr) {
  std::atomic<bool> started{false};
  std::atomic<bool> cancelled{false};
  auto shared = std::make_shared<int>(1);
  {
    concurrent::BackgroundTaskStorage bts;

    bts.AsyncDetach("test", [shared, &started, &cancelled] {
      started = true;

      engine::SingleConsumerEvent event;
      cancelled = !event.WaitForEventFor(utest::kMaxTestWaitTime);
    });

    engine::SingleConsumerEvent event;
    EXPECT_FALSE(event.WaitForEventFor(50ms));

    EXPECT_EQ(shared.use_count(), 2);
  }

  EXPECT_TRUE(started);
  EXPECT_TRUE(cancelled);
  EXPECT_EQ(shared.use_count(), 1);
}

UTEST(BackgroundTaskStorage, Sample) {
  constexpr auto kString = "a very-very-very-very long string";
  const auto DoStuff = [] {
    // Make sure the task starts
    engine::Yield();
  };

  /// [Sample]
  std::string x = kString;
  std::string y;
  {
    // You must guarantee that 'x' and 'y' are alive during 'bts' lifetime.
    concurrent::BackgroundTaskStorage bts;

    bts.AsyncDetach("task", [&x, &y] {
      engine::InterruptibleSleepFor(20s);
      EXPECT_TRUE(engine::current_task::ShouldCancel());
      y = std::move(x);
    });

    DoStuff();

    // It is recommended to call CancelAndWait explicitly for the clarity of
    // tasks' lifetime. BTS destructor calls CancelAndWait implicitly as well.
    bts.CancelAndWait();
  }
  /// [Sample]

  EXPECT_EQ(x, "");
  EXPECT_EQ(y, kString);
}

UTEST(BackgroundTaskStorage, NoDeadlockWithUnstartedTasks) {
  concurrent::BackgroundTaskStorage bts;
  bts.AsyncDetach("test", [] {
    engine::SingleConsumerEvent event;

    EXPECT_FALSE(event.WaitForEvent());
  });
}

UTEST(BackgroundTaskStorage, MutableLambda) {
  concurrent::BackgroundTaskStorage bts;
  bts.AsyncDetach("test", [value = 1]() mutable {
    ++value;
    return value;
  });
}

UTEST(BackgroundTaskStorage, ActiveTasksCounter) {
  const long kNoopTasks = 2;
  const long kLongTasks = 3;
  concurrent::BackgroundTaskStorage bts;

  for (int i = 0; i < kNoopTasks; ++i) {
    bts.AsyncDetach("noop-task", [] { /* noop */ });
  }
  for (int i = 0; i < kLongTasks; ++i) {
    bts.AsyncDetach("long-task", [] {
      engine::SingleConsumerEvent event;

      EXPECT_FALSE(event.WaitForEventFor(2s));
    });
  }

  EXPECT_EQ(bts.ActiveTasksApprox(), kNoopTasks + kLongTasks);

  engine::SingleConsumerEvent event;
  EXPECT_FALSE(event.WaitForEventFor(50ms));

  EXPECT_EQ(bts.ActiveTasksApprox(), kLongTasks);
}

UTEST(BackgroundTaskStorage, ExceptionWhilePreparingTask) {
  concurrent::BackgroundTaskStorage bts;

  const auto func = [](int x) { EXPECT_EQ(x, 42); };
  utils::LazyPrvalue x([]() -> int {
    throw std::runtime_error("exception during arg in-place construction");
  });

  UEXPECT_THROW(bts.AsyncDetach("test", func, std::move(x)),
                std::runtime_error);

  // The destruction of 'bts' must not cause data races, hang or leak memory
}

UTEST(BackgroundTaskStorage, CancelAndWait) {
  std::atomic<bool> finished{false};
  concurrent::BackgroundTaskStorage bts;

  bts.AsyncDetach("", [&] {
    engine::InterruptibleSleepFor(utest::kMaxTestWaitTime);
    finished = true;
  });

  // Make sure the task starts
  engine::Yield();

  bts.CancelAndWait();
  EXPECT_TRUE(finished);
}

UTEST(BackgroundTaskStorage, SleepWhileCancelled) {
  concurrent::BackgroundTaskStorageCore bts;
  engine::SingleConsumerEvent event;

  bts.Detach(utils::CriticalAsync("", [&] {
    engine::SleepFor(10ms);
    event.Send();
  }));

  bts.CancelAndWait();
  EXPECT_TRUE(event.WaitForEventFor(utest::kMaxTestWaitTime));
}

UTEST(BackgroundTaskStorage, DetachWhileWaiting) {
  std::atomic<bool> finished{false};
  concurrent::BackgroundTaskStorageCore bts;

  bts.Detach(utils::CriticalAsync("outer", [&] {
    // Make sure the main task enters CancelAndWait
    engine::Yield();

    bts.Detach(utils::CriticalAsync("inner", [&] { finished = true; }));
  }));

  bts.CancelAndWait();
  EXPECT_TRUE(finished);
}

UTEST(BackgroundTaskStorage, Pimpl) {
  concurrent::BackgroundTaskStorageFastPimpl bts;
  EXPECT_EQ(bts->ActiveTasksApprox(), 0);
}

namespace {
engine::TaskInheritedVariable<int> inherited_variable;
}  // namespace

UTEST(BackgroundTaskStorage, NoTaskInheritedVariables) {
  inherited_variable.Set(42);
  EXPECT_EQ(inherited_variable.Get(), 42);

  concurrent::BackgroundTaskStorage bts;
  bts.AsyncDetach("", [] { EXPECT_FALSE(inherited_variable.GetOptional()); });
}

TEST(BackgroundTaskStorage, CustomTaskProcessor) {
  engine::TwoStandaloneTaskProcessors tp;
  tp.RunBlocking([&] {
    engine::SingleConsumerEvent finished;
    concurrent::BackgroundTaskStorage bts(tp.GetSecondary());

    bts.AsyncDetach("", [&] {
      EXPECT_EQ(&engine::current_task::GetTaskProcessor(), &tp.GetSecondary());
      finished.Send();
    });

    EXPECT_TRUE(finished.WaitForEvent());
  });
}

TEST(BackgroundTaskStorage, StrongTaskProcessorBinding) {
  engine::TwoStandaloneTaskProcessors tp;
  tp.RunBlocking([&] {
    engine::SingleConsumerEvent finished;
    concurrent::BackgroundTaskStorage bts;

    engine::AsyncNoSpan(tp.GetSecondary(), [&] {
      bts.AsyncDetach("", [&] {
        EXPECT_EQ(&engine::current_task::GetTaskProcessor(), &tp.GetMain());
        finished.Send();
      });
    }).Get();

    EXPECT_TRUE(finished.WaitForEvent());
  });
}

USERVER_NAMESPACE_END
