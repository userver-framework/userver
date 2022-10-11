#include <userver/utest/utest.hpp>

#include <cerrno>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <vector>

#include <userver/engine/async.hpp>
#include <userver/engine/deadline.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/engine/task/task_with_result.hpp>

USERVER_NAMESPACE_BEGIN

namespace {
constexpr std::size_t kNumThreads = 3;
}  // namespace

UTEST_MT(Errno, IsCoroLocal, kNumThreads) {
  std::size_t threads_started{0};
  std::size_t threads_switched{0};
  std::mutex mutex;
  std::condition_variable cv;
  std::vector<engine::TaskWithResult<bool>> tasks;

  auto deadline =
      engine::Deadline::FromDuration(std::chrono::milliseconds{100});

  for (size_t i = 0; i < kNumThreads; ++i) {
    tasks.push_back(engine::AsyncNoSpan([&] {
      {
        // ensure every task gets its own thread
        std::unique_lock lock(mutex);
        ++threads_started;
        cv.wait_for(lock, deadline.TimeLeft(),
                    [&] { return threads_started >= kNumThreads; });
        cv.notify_all();
      }

      const auto init_thread_id = std::this_thread::get_id();
      const auto* init_errno_ptr = &errno;
      // if attribute `const` is used all subsequent `errno` TLS accesses
      // may be optimized out

      while (!engine::current_task::IsCancelRequested() &&
             !deadline.IsReached()) {
        if (std::this_thread::get_id() != init_thread_id) {
          // lock this thread until all others switch as well
          std::unique_lock lock(mutex);
          ++threads_switched;
          cv.wait_for(lock, deadline.TimeLeft(),
                      [&] { return threads_switched >= kNumThreads; });
          cv.notify_all();

          return &errno != init_errno_ptr;
        } else {
          engine::Yield();
        }
      }
      return true;
    }));
  }

  for (auto& task : tasks) {
    EXPECT_TRUE(task.Get());
  }
}

USERVER_NAMESPACE_END
