#include <utest/utest.hpp>

#include <engine/async.hpp>
#include <utils/work_control.hpp>

namespace {
constexpr auto kPollInterval = std::chrono::milliseconds(50);
}

TEST(WorkControl, Ctr) {
  RunInCoro([] {
    utils::WorkControl work_control;
    EXPECT_FALSE(work_control.AreWorkersAlive());
  });
}

TEST(WorkControl, GetAndStop) {
  RunInCoro([] {
    utils::WorkControl work_control;

    work_control.GetToken();
    EXPECT_FALSE(work_control.AreWorkersAlive());
  });
}

TEST(WorkControl, GetAndCheck) {
  RunInCoro([] {
    utils::WorkControl work_control;

    auto token = work_control.GetToken();
    EXPECT_TRUE(work_control.AreWorkersAlive());

    token.reset();
    EXPECT_FALSE(work_control.AreWorkersAlive());
  });
}

TEST(WorkControl, WaitAsync) {
  RunInCoro([] {
    utils::WorkControl work_control;

    auto token = work_control.GetToken();
    EXPECT_TRUE(work_control.AreWorkersAlive());

    auto task = engine::impl::Async([token = std::move(token)] {
      while (!token->IsFinished()) engine::Yield();
    });
    EXPECT_TRUE(work_control.AreWorkersAlive());

    engine::Yield();
    engine::Yield();
    EXPECT_TRUE(work_control.AreWorkersAlive());

    work_control.SetFinished();
    task.Get();
    EXPECT_FALSE(work_control.AreWorkersAlive());
  });
}

TEST(WorkControl, WaitSync) {
  RunInCoro([] {
    utils::WorkControl work_control;

    auto token = work_control.GetToken();
    EXPECT_TRUE(work_control.AreWorkersAlive());

    auto task = engine::impl::Async([token = std::move(token)] {
      while (!token->IsFinished()) engine::Yield();
    });
    EXPECT_TRUE(work_control.AreWorkersAlive());

    engine::Yield();
    engine::Yield();
    EXPECT_TRUE(work_control.AreWorkersAlive());

    work_control.FinishAndWaitForAllWorkers(kPollInterval);
    EXPECT_FALSE(work_control.AreWorkersAlive());
  });
}

TEST(WorkControl, WaitSync2) {
  RunInCoro([] {
    utils::WorkControl work_control;

    auto task1 = engine::impl::Async([token = work_control.GetToken()] {
      while (!token->IsFinished()) engine::Yield();
    });
    auto task2 = engine::impl::Async([token = work_control.GetToken()] {
      while (!token->IsFinished()) engine::Yield();
    });
    EXPECT_TRUE(work_control.AreWorkersAlive());

    engine::Yield();
    engine::Yield();
    EXPECT_TRUE(work_control.AreWorkersAlive());

    work_control.FinishAndWaitForAllWorkers(kPollInterval);
    EXPECT_FALSE(work_control.AreWorkersAlive());
  });
}
