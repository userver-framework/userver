#include <utest/utest.hpp>

#include <userver/engine/async.hpp>
#include <userver/utils/work_control.hpp>

namespace {
constexpr auto kPollInterval = std::chrono::milliseconds(50);
}

UTEST(WorkControl, Ctr) {
  utils::WorkControl work_control;
  EXPECT_FALSE(work_control.AreWorkersAlive());
}

UTEST(WorkControl, GetAndStop) {
  utils::WorkControl work_control;

  work_control.GetToken();
  EXPECT_FALSE(work_control.AreWorkersAlive());
}

UTEST(WorkControl, GetAndCheck) {
  utils::WorkControl work_control;

  auto token = work_control.GetToken();
  EXPECT_TRUE(work_control.AreWorkersAlive());

  token.reset();
  EXPECT_FALSE(work_control.AreWorkersAlive());
}

UTEST(WorkControl, WaitAsync) {
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
}

UTEST(WorkControl, WaitSync) {
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
}

UTEST(WorkControl, WaitSync2) {
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
}
