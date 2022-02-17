#include <userver/utest/utest.hpp>

#include <userver/engine/get_all.hpp>
#include <userver/engine/single_consumer_event.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/utils/async.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

engine::TaskWithResult<void> SlowSuccessfulTask(bool ensure_cancelled = true) {
  return utils::Async("success_slow", [ensure_cancelled] {
    engine::SingleConsumerEvent event;
    EXPECT_FALSE(event.WaitForEventFor(std::chrono::milliseconds{1000}));

    if (ensure_cancelled) {
      engine::current_task::CancellationPoint();
      FAIL();
    }
  });
}

engine::TaskWithResult<void> FastFailingTask() {
  return utils::Async("fail_fast", [] {
    engine::SingleConsumerEvent event;
    EXPECT_FALSE(event.WaitForEventFor(std::chrono::milliseconds{20}));

    throw std::runtime_error{"failfast_exception"};
  });
}

engine::TaskWithResult<void> FastSuccessfulTask() {
  return utils::Async("success_fast", [] {
    engine::SingleConsumerEvent event;
    EXPECT_FALSE(event.WaitForEventFor(std::chrono::milliseconds{20}));
  });
}

}  // namespace

UTEST(GetAll, JustWorksVectorTasks) {
  std::vector<engine::TaskWithResult<void>> tasks;
  for (size_t i = 0; i < 4; ++i) {
    tasks.emplace_back(FastSuccessfulTask());
  }

  engine::GetAll(tasks);

  for (auto& task : tasks) EXPECT_FALSE(task.IsValid());
}

UTEST(GetAll, JustWorksVariadicTasks) {
  auto first_task = FastSuccessfulTask();
  auto second_task = FastSuccessfulTask();
  auto third_task = FastSuccessfulTask();

  engine::GetAll(first_task, second_task, third_task);

  EXPECT_FALSE(first_task.IsValid());
  EXPECT_FALSE(second_task.IsValid());
  EXPECT_FALSE(third_task.IsValid());
}

UTEST_MT(GetAll, EarlyThrow, 4) {
  std::vector<engine::TaskWithResult<void>> tasks;
  for (size_t i = 0; i < 4; ++i) {
    tasks.emplace_back(SlowSuccessfulTask());
  }
  tasks.emplace_back(FastFailingTask());

  EXPECT_THROW(engine::GetAll(tasks), std::runtime_error);
}

UTEST_DEATH(GetAllDeathTest, InvalidTask) {
  std::vector<engine::TaskWithResult<void>> tasks;
  tasks.emplace_back(engine::TaskWithResult<void>{});
  EXPECT_UINVARIANT_FAILURE(engine::GetAll(tasks));

  tasks.resize(0);
  tasks.emplace_back(SlowSuccessfulTask());
  for (size_t i = 0; i < 3; ++i) {
    tasks.emplace_back(engine::TaskWithResult<void>{});
  }
  EXPECT_UINVARIANT_FAILURE(engine::GetAll(tasks));
}

UTEST(GetAll, Cancellation) {
  std::vector<engine::TaskWithResult<void>> tasks;
  for (size_t i = 0; i < 4; ++i) tasks.emplace_back(SlowSuccessfulTask());

  engine::current_task::SetDeadline(
      engine::Deadline::FromTimePoint(engine::Deadline::kPassed));

  EXPECT_THROW(engine::GetAll(tasks), engine::WaitInterruptedException);
}

UTEST(GetAll, SequentialWakeups) {
  const size_t tasks_count = 10;

  std::vector<std::unique_ptr<engine::SingleConsumerEvent>> events;
  events.reserve(tasks_count);
  for (size_t i = 0; i < tasks_count; ++i) {
    events.emplace_back(std::make_unique<engine::SingleConsumerEvent>());
  }

  std::vector<engine::TaskWithResult<void>> tasks;
  tasks.reserve(tasks_count);
  for (size_t i = 0; i < tasks_count; ++i) {
    tasks.emplace_back(engine::AsyncNoSpan([i, &events] {
      if (i + 1 < tasks_count) {
        ASSERT_TRUE(events[i + 1]->WaitForEventFor(kMaxTestWaitTime));
      }
      events[i]->Send();
    }));
  }

  engine::GetAll(tasks);
}

USERVER_NAMESPACE_END
