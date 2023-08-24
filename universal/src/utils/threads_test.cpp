#include <userver/utils/threads.hpp>

#include <sys/resource.h>
#include <thread>

#include <gtest/gtest.h>

USERVER_NAMESPACE_BEGIN

TEST(Threads, MainThreadLowPriority) {
  const int prev_priority = ::getpriority(PRIO_PROCESS, 0);
  utils::SetCurrentThreadLowPriorityScheduling();
  const int new_priority = ::getpriority(PRIO_PROCESS, 0);
  EXPECT_LE(prev_priority, new_priority);
}

TEST(Threads, NotMainThreadLowPriority) {
  const int main_priority = ::getpriority(PRIO_PROCESS, 0);

  std::thread another_thread([] {
    const int prev_priority = ::getpriority(PRIO_PROCESS, 0);
    utils::SetCurrentThreadLowPriorityScheduling();
    const int new_priority = ::getpriority(PRIO_PROCESS, 0);
    EXPECT_LE(prev_priority, new_priority);
  });
  another_thread.join();

  EXPECT_EQ(main_priority, ::getpriority(PRIO_PROCESS, 0));
}

USERVER_NAMESPACE_END
