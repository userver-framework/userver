#include <gtest/gtest.h>

#include <chrono>
#include <exception>
#include <stdexcept>

#include <userver/engine/async.hpp>
#include <userver/engine/condition_variable.hpp>
#include <userver/engine/mutex.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/utest/utest.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

class TestException : public std::exception {};

}  // namespace

UTEST(CxxabiEhGlobals, UncaughtIsCoroLocal) {
  try {
    engine::Mutex mutex;
    engine::ConditionVariable cv;
    std::unique_lock<engine::Mutex> lock(mutex);

    auto subtask = engine::AsyncNoSpan([&cv, &mutex] {
      {
        std::unique_lock<engine::Mutex> lock(mutex);
        cv.NotifyOne();
      }
      engine::SleepFor(std::chrono::seconds(1));
      engine::current_task::CancellationPoint();

      // if we got here, subtask wasn't cancelled while it should've been
      // one of possible reasons is uncaught exception leaked via thread local
      ASSERT_FALSE(std::uncaught_exceptions());
      FAIL() << "Subtask wasn't cancelled";
    });
    ASSERT_EQ(engine::CvStatus::kNoTimeout, cv.Wait(lock));

    // we'll switch to subtask during stack unwinding (in its dtor)
    throw TestException{};
  } catch (const TestException&) {
    return;
  }
  FAIL() << "Exception has been lost";
}

UTEST(CxxabiEhGlobals, ActiveIsCoroLocal) {
  engine::Mutex mutex;
  engine::ConditionVariable cv;
  engine::ConditionVariable sub_cv;
  std::unique_lock<engine::Mutex> lock(mutex);

  auto subtask = engine::AsyncNoSpan([&cv, &mutex, &sub_cv] {
    std::unique_lock<engine::Mutex> lock(mutex);
    cv.NotifyOne();
    ASSERT_EQ(engine::CvStatus::kNoTimeout, sub_cv.Wait(lock));

    // this coro shouldn't have an active exception
    ASSERT_FALSE(std::current_exception());
    cv.NotifyOne();
  });
  ASSERT_EQ(engine::CvStatus::kNoTimeout, cv.Wait(lock));

  try {
    throw TestException{};
  } catch (const TestException&) {
    sub_cv.NotifyOne();
    cv.WaitFor(lock, std::chrono::seconds(1));

    // this coro didn't lose an active exception
    ASSERT_TRUE(std::current_exception());
  }
}

USERVER_NAMESPACE_END
