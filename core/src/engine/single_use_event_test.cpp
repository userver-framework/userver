#include <userver/engine/single_use_event.hpp>

#include <atomic>
#include <thread>
#include <vector>

#include <boost/lockfree/queue.hpp>

#include <userver/engine/async.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/engine/task/task_with_result.hpp>
#include <userver/engine/wait_any.hpp>
#include <userver/utest/utest.hpp>
#include <userver/utils/async.hpp>
#include <userver/utils/fixed_array.hpp>

using namespace std::chrono_literals;

USERVER_NAMESPACE_BEGIN

TEST(SingleUseEvent, UnusedEvent) { engine::SingleUseEvent event; }

UTEST(SingleUseEvent, IsReady) {
  engine::SingleUseEvent event;
  event.Send();
  EXPECT_TRUE(event.IsReady());
  EXPECT_TRUE(event.IsReady());

  EXPECT_EQ(event.WaitUntil(engine::Deadline{}), engine::FutureStatus::kReady);
  EXPECT_TRUE(event.IsReady());
}

UTEST(SingleUseEvent, WaitAndSend) {
  engine::SingleUseEvent event;
  auto task = engine::AsyncNoSpan([&] { UEXPECT_NO_THROW(event.Wait()); });

  engine::Yield();  // force a context switch to 'task'
  EXPECT_FALSE(task.IsFinished());

  event.Send();
  UEXPECT_NO_THROW(task.WaitFor(utest::kMaxTestWaitTime));
}

UTEST(SingleUseEvent, SendAndWait) {
  engine::SingleUseEvent event;
  std::atomic<bool> is_event_sent{false};

  auto task = engine::AsyncNoSpan([&] {
    while (!is_event_sent) engine::Yield();
    UEXPECT_NO_THROW(event.Wait());
  });

  event.Send();
  is_event_sent = true;

  UEXPECT_NO_THROW(task.WaitFor(utest::kMaxTestWaitTime));
}

UTEST(SingleUseEvent, Sample) {
  /// [Wait and destroy]
  engine::TaskWithResult<void> sender;
  {
    engine::SingleUseEvent event;
    sender = utils::Async("sender", [&event] { event.Send(); });

    event.WaitNonCancellable();  // will be woken up by 'Send()' above

    // 'event' is destroyed here. Note that 'Send' might continue executing, but
    // it will still complete safely.
  }
  /// [Wait and destroy]
  sender.Get();
}

UTEST_MT(SingleUseEvent, SimpleTaskQueue, 5) {
  struct SimpleTask final {
    std::uint64_t request;
    std::uint64_t response;
    engine::SingleUseEvent completion;
  };

  boost::lockfree::queue<SimpleTask*> task_queue{1};

  std::atomic<bool> keep_running_clients{true};
  std::atomic<bool> keep_running_server{true};

  std::vector<engine::TaskWithResult<void>> client_tasks;
  client_tasks.reserve(GetThreadCount() - 1);

  for (std::size_t i = 0; i < GetThreadCount() - 1; ++i) {
    client_tasks.push_back(utils::Async("client", [&, i] {
      std::uint64_t request = i;

      while (keep_running_clients) {
        SimpleTask task{request, {}, {}};
        task_queue.push(&task);
        task.completion.WaitNonCancellable();
        EXPECT_EQ(task.response, task.request * 2);
        request += GetThreadCount();
        // `task` is destroyed here, with SingleConsumerEvent UB would occur
      }
    }));
  }

  auto server_task = utils::Async("server", [&] {
    while (keep_running_server) {
      SimpleTask* task{};
      if (!task_queue.pop(task)) continue;

      task->response = task->request * 2;
      task->completion.Send();
    }
  });

  engine::SleepFor(50ms);

  keep_running_clients = false;
  for (auto& task : client_tasks) task.Get();
  keep_running_server = false;
  server_task.Get();
}

UTEST(SingleUseEvent, Cancellation) {
  engine::SingleUseEvent event;

  auto waiter = engine::CriticalAsyncNoSpan([&event] {
    const auto deadline =
        engine::Deadline::FromDuration(utest::kMaxTestWaitTime);
    EXPECT_EQ(event.WaitUntil(deadline), engine::FutureStatus::kCancelled);
    UEXPECT_THROW(event.Wait(), engine::WaitInterruptedException);
    return engine::current_task::ShouldCancel();
  });

  waiter.SyncCancel();
  UEXPECT_NO_THROW(EXPECT_TRUE(waiter.Get()));
}

UTEST(SingleUseEvent, Deadline) {
  engine::SingleUseEvent event;

  auto waiter = engine::AsyncNoSpan([&event] {
    const auto deadline = engine::Deadline::FromDuration(10ms);
    EXPECT_EQ(event.WaitUntil(deadline), engine::FutureStatus::kTimeout);
    return deadline.IsReached();
  });

  UEXPECT_NO_THROW(EXPECT_TRUE(waiter.Get()));
}

UTEST_MT(SingleUseEvent, SendWaitRace, 2) {
  const auto test_deadline = engine::Deadline::FromDuration(100ms);

  while (!test_deadline.IsReached()) {
    engine::SingleUseEvent event;

    auto waiter = engine::CriticalAsyncNoSpan([&event] {
      std::this_thread::yield();
      return event.WaitUntil(engine::Deadline{});
    });

    std::this_thread::yield();
    event.Send();
    ASSERT_EQ(waiter.Get(), engine::FutureStatus::kReady);
  }
}

UTEST_MT(SingleUseEvent, SendCancelRace, 3) {
  const auto test_deadline = engine::Deadline::FromDuration(100ms);

  bool is_ready_status_achieved = false;
  bool is_cancel_status_achieved = false;

  while (!test_deadline.IsReached() || !is_ready_status_achieved ||
         !is_cancel_status_achieved) {
    engine::SingleUseEvent event;

    auto waiter = engine::CriticalAsyncNoSpan([&event] {
      std::this_thread::yield();
      return event.WaitUntil(engine::Deadline{});
    });

    auto canceller = engine::CriticalAsyncNoSpan([&waiter] {
      std::this_thread::yield();
      waiter.RequestCancel();
    });

    std::this_thread::yield();
    event.Send();

    UASSERT_NO_THROW(canceller.Get());

    const auto status = waiter.Get();
    switch (status) {
      case engine::FutureStatus::kReady:
        is_ready_status_achieved = true;
        break;
      case engine::FutureStatus::kCancelled:
        is_cancel_status_achieved = true;
        break;
      default:
        GTEST_FAIL();
    }
  }
}

namespace {

// The param tells which event to notify.
class SingleUseEventWaitAny : public testing::TestWithParam<std::size_t> {};

constexpr std::size_t kEventCount = 3;

}  // namespace

INSTANTIATE_UTEST_SUITE_P(
    /*no prefix*/, SingleUseEventWaitAny,
    ::testing::Range(std::size_t{0}, kEventCount));

UTEST_P(SingleUseEventWaitAny, Basic) {
  const auto event_to_notify = GetParam();
  auto events = utils::FixedArray<engine::SingleUseEvent>(kEventCount);

  auto notifier = engine::AsyncNoSpan([&events, event_to_notify] {
    engine::SleepFor(50ms);
    events[event_to_notify].Send();
  });

  const auto wait_result = engine::WaitAnyFor(utest::kMaxTestWaitTime, events);
  EXPECT_EQ(wait_result, event_to_notify);

  UEXPECT_NO_THROW(notifier.Get());
}

UTEST_P_MT(SingleUseEventWaitAny, WaitSendRace, 2) {
  const auto event_to_notify = GetParam();
  const auto test_deadline = engine::Deadline::FromDuration(50ms);

  while (!test_deadline.IsReached()) {
    auto events = utils::FixedArray<engine::SingleUseEvent>(kEventCount);

    auto notifier = engine::AsyncNoSpan([&events, event_to_notify] {
      std::this_thread::yield();
      events[event_to_notify].Send();
    });

    std::this_thread::yield();
    const auto index = engine::WaitAny(events);
    ASSERT_EQ(index, event_to_notify);

    UASSERT_NO_THROW(notifier.Get());
  }
}

UTEST_P_MT(SingleUseEventWaitAny, SendCancelRace, 3) {
  const auto event_to_notify = GetParam();
  const auto test_deadline = engine::Deadline::FromDuration(50ms);

  bool is_ready_status_achieved = false;
  bool is_cancel_status_achieved = false;

  while (!test_deadline.IsReached() || !is_ready_status_achieved ||
         !is_cancel_status_achieved) {
    auto events = utils::FixedArray<engine::SingleUseEvent>(kEventCount);

    auto waiter = engine::CriticalAsyncNoSpan([&events] {
      std::this_thread::yield();
      return engine::WaitAny(events);
    });

    auto canceller = engine::CriticalAsyncNoSpan([&waiter] {
      std::this_thread::yield();
      waiter.RequestCancel();
    });

    std::this_thread::yield();
    events[event_to_notify].Send();

    UASSERT_NO_THROW(canceller.Get());

    const auto status = waiter.Get();
    if (status == event_to_notify) {
      is_ready_status_achieved = true;
    } else if (status == std::nullopt) {
      is_cancel_status_achieved = true;
    } else {
      GTEST_FAIL();
    }
  }
}

USERVER_NAMESPACE_END
