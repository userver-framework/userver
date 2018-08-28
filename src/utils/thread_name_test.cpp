#include <gtest/gtest.h>

#include <atomic>
#include <condition_variable>
#include <mutex>

#include <main_test.hpp>
#include <utils/thread_name.hpp>

TEST(ThreadName, Set) {
  std::mutex mutex;
  std::condition_variable cv;
  std::atomic_int c{0};

  auto f = [&]() {
    std::unique_lock<std::mutex> lock(mutex);
    cv.wait(lock, [&c]() { return c.load() > 0; });
  };
  auto thread = std::thread(f);

  auto old_name = utils::GetThreadName(thread);
  auto new_name = "12345";

  utils::SetThreadName(thread, new_name);
  EXPECT_EQ(new_name, utils::GetThreadName(thread));

  utils::SetThreadName(thread, old_name);
  EXPECT_EQ(old_name, utils::GetThreadName(thread));

  {
    std::lock_guard<std::mutex> lock(mutex);
    c++;
    cv.notify_all();
  }

  thread.join();
}
