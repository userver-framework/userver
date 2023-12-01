#include <userver/utest/utest.hpp>

#include <userver/engine/async.hpp>
#include <userver/engine/condition_variable.hpp>
#include <userver/engine/mutex.hpp>
#include <userver/engine/run_standalone.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/formats/json/value.hpp>

USERVER_NAMESPACE_BEGIN

UTEST_MT(RacyLazyAllocation, DoesntHappen, 3) {
  for (std::size_t i = 0; i < 1'000; ++i) {
    engine::Mutex m{};
    engine::ConditionVariable cv{};
    bool ok = false;

    const formats::json::Value value{};

    auto is_null_task = engine::AsyncNoSpan([&m, &cv, &value, &ok] {
      {
        std::unique_lock lock{m};
        EXPECT_TRUE(cv.Wait(lock, [&ok] { return ok; }));
      }

      EXPECT_TRUE(value.IsNull());
    });

    auto is_object_task = engine::AsyncNoSpan([&m, &cv, &value, &ok] {
      {
        std::unique_lock lock{m};
        EXPECT_TRUE(cv.Wait(lock, [&ok] { return ok; }));
      }

      EXPECT_FALSE(value.IsObject());
    });

    engine::Yield();

    {
      const std::unique_lock lock{m};
      ok = true;
    }
    cv.NotifyAll();

    is_object_task.Get();
    is_null_task.Get();
  }
}

USERVER_NAMESPACE_END
