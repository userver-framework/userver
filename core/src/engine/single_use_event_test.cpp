#include <userver/engine/single_use_event.hpp>

#include <atomic>
#include <vector>

#include <boost/lockfree/queue.hpp>

#include <userver/engine/sleep.hpp>
#include <userver/engine/task/task_with_result.hpp>
#include <userver/utest/utest.hpp>
#include <userver/utils/async.hpp>

USERVER_NAMESPACE_BEGIN

TEST(SingleUseEvent, UnusedEvent) { engine::SingleUseEvent event; }

UTEST(SingleUseEvent, WaitAndSend) {
  engine::SingleUseEvent event;
  auto task = utils::Async(
      "waiter", [&] { UEXPECT_NO_THROW(event.WaitNonCancellable()); });

  engine::Yield();  // force a context switch to 'task'
  EXPECT_FALSE(task.IsFinished());

  event.Send();
  UEXPECT_NO_THROW(task.WaitFor(utest::kMaxTestWaitTime));
}

UTEST(SingleUseEvent, SendAndWait) {
  engine::SingleUseEvent event;
  std::atomic<bool> is_event_sent{false};

  auto task = utils::Async("waiter", [&] {
    while (!is_event_sent) engine::Yield();
    UEXPECT_NO_THROW(event.WaitNonCancellable());
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

UTEST(SingleUseEvent, WaitAndSendDouble) {
  engine::SingleUseEvent event;
  engine::SingleUseEvent event_has_been_reset;

  auto task = utils::Async("waiter", [&] {
    event.WaitNonCancellable();

    event.Reset();
    event_has_been_reset.Send();

    event.WaitNonCancellable();
  });

  // By this time, 'task' may or may not have entered 'WaitNonCancellable'
  event.Send();
  EXPECT_FALSE(task.IsFinished());

  // 'SingleUseEvent' cannot be used twice in the same interaction. In this
  // test, we use another 'SingleUseEvent' to make sure that the first signal
  // has been received.
  event_has_been_reset.WaitNonCancellable();
  EXPECT_FALSE(task.IsFinished());

  event.Send();
  UEXPECT_NO_THROW(task.WaitFor(utest::kMaxTestWaitTime));
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

  engine::SleepFor(std::chrono::milliseconds{50});

  keep_running_clients = false;
  for (auto& task : client_tasks) task.Get();
  keep_running_server = false;
  server_task.Get();
}

USERVER_NAMESPACE_END
