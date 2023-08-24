#include <engine/ev/watcher/timer_watcher.hpp>

#include <engine/ev/thread.hpp>
#include <userver/utest/utest.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

constexpr auto kDefaultTimerMode =
    engine::ev::Thread::RegisterEventMode::kImmediate;

}

UTEST(TimerWatcher, SingleShot) {
  engine::ev::Thread thread("test_thread", kDefaultTimerMode);
  engine::ev::ThreadControl thread_control(thread);

  std::mutex mutex;
  std::condition_variable cv;
  std::error_code ec;
  std::atomic_int c{0};

  engine::ev::TimerWatcher watcher(thread_control);
  watcher.SingleshotAsync(std::chrono::milliseconds(50),
                          [&ec, &c, &mutex, &cv](std::error_code ec_arg) {
                            std::lock_guard<std::mutex> lock(mutex);
                            c++;
                            ec = ec_arg;
                            cv.notify_all();
                          });
  EXPECT_EQ(0, c.load());

  {
    std::unique_lock<std::mutex> lock(mutex);
    cv.wait_for(lock, std::chrono::milliseconds(200),
                [&c]() { return c.load() > 0; });
  }

  EXPECT_EQ(1, c.load());
  EXPECT_EQ(std::error_code(), ec);
}

UTEST(TimerWatcher, Cancel) {
  engine::ev::Thread thread("test_thread", kDefaultTimerMode);
  engine::ev::ThreadControl thread_control(thread);

  std::mutex mutex;
  std::condition_variable cv;
  std::atomic_int c{0};
  std::error_code ec;

  engine::ev::TimerWatcher watcher(thread_control);
  watcher.SingleshotAsync(std::chrono::milliseconds(50),
                          [&c, &ec, &mutex, &cv](std::error_code ec_arg) {
                            std::lock_guard<std::mutex> lock(mutex);
                            c++;
                            ec = ec_arg;
                            cv.notify_all();
                          });
  EXPECT_EQ(0, c.load());
  watcher.Cancel();

  {
    std::unique_lock<std::mutex> lock(mutex);
    cv.wait_for(lock, std::chrono::milliseconds(200),
                [&c]() { return c.load() > 0; });
  }

  EXPECT_EQ(1, c.load());
  EXPECT_EQ(std::make_error_code(std::errc::operation_canceled), ec);
}

UTEST(TimerWatcher, CancelAfterExpire) {
  engine::ev::Thread thread("test_thread", kDefaultTimerMode);
  engine::ev::ThreadControl thread_control(thread);

  std::mutex mutex;
  std::condition_variable cv;
  std::atomic_int c{0};
  std::error_code ec;

  engine::ev::TimerWatcher watcher(thread_control);
  watcher.SingleshotAsync(std::chrono::milliseconds(50),
                          [&c, &ec, &mutex, &cv](std::error_code ec_arg) {
                            std::lock_guard<std::mutex> lock(mutex);
                            c++;
                            ec = ec_arg;
                            cv.notify_all();
                          });
  EXPECT_EQ(0, c.load());

  {
    std::unique_lock<std::mutex> lock(mutex);
    cv.wait_for(lock, std::chrono::milliseconds(200),
                [&c]() { return c.load() > 0; });
  }

  EXPECT_EQ(1, c.load());
  EXPECT_EQ(std::error_code(), ec);

  watcher.Cancel();
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  // c and ec shouldn't change
  EXPECT_EQ(1, c.load());
  EXPECT_EQ(std::error_code(), ec);
}

UTEST(TimerWatcher, CreateAndCancel) {
  engine::ev::Thread thread("test_thread", kDefaultTimerMode);
  engine::ev::ThreadControl thread_control(thread);

  engine::ev::TimerWatcher watcher(thread_control);

  watcher.Cancel();
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  // c and ec shouldn't change
  EXPECT_EQ(1, 1);
}

USERVER_NAMESPACE_END
