#include <userver/os_signals/processor_mock.hpp>

#include <userver/utest/utest.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

struct Helper {
  void OnSigUsr1() { ++sigusr1; }

  void OnSigUsrX(int signum) {
    if (signum == os_signals::kSigUsr1) {
      ++sigusr1;
    } else {
      ASSERT_EQ(signum, os_signals::kSigUsr2);
      ++sigusr2;
    }
  }

  std::size_t sigusr1{0};
  std::size_t sigusr2{0};
};

}  // namespace

UTEST(OsSignals, Basic) {
  os_signals::ProcessorMock proc{engine::current_task::GetTaskProcessor()};
  UASSERT_NO_THROW(proc.Notify(os_signals::kSigUsr1));
  UASSERT_NO_THROW(proc.Notify(os_signals::kSigUsr2));

  Helper h1;
  Helper h2;

  const auto subscribe1 = proc.Get().AddListener(
      &h1, "h1", os_signals::kSigUsr1, &Helper::OnSigUsr1);
  const auto subscribe2 = proc.Get().AddListener(
      &h2, "h2", os_signals::kSigUsr1, &Helper::OnSigUsr1);
  EXPECT_EQ(h1.sigusr1, 0);
  EXPECT_EQ(h1.sigusr2, 0);
  EXPECT_EQ(h2.sigusr1, 0);
  EXPECT_EQ(h2.sigusr2, 0);

  proc.Notify(os_signals::kSigUsr1);
  EXPECT_EQ(h1.sigusr1, 1);
  EXPECT_EQ(h1.sigusr2, 0);
  EXPECT_EQ(h2.sigusr1, 1);
  EXPECT_EQ(h2.sigusr2, 0);

  proc.Notify(os_signals::kSigUsr1);
  EXPECT_EQ(h1.sigusr1, 2);
  EXPECT_EQ(h1.sigusr2, 0);
  EXPECT_EQ(h2.sigusr1, 2);
  EXPECT_EQ(h2.sigusr2, 0);

  // We were not subscribed to os_signals::kSigUsr2
  proc.Notify(os_signals::kSigUsr2);
  EXPECT_EQ(h1.sigusr1, 2);
  EXPECT_EQ(h1.sigusr2, 0);
  EXPECT_EQ(h2.sigusr1, 2);
  EXPECT_EQ(h2.sigusr2, 0);
}

UTEST(OsSignals, BothSignals) {
  os_signals::ProcessorMock proc{engine::current_task::GetTaskProcessor()};
  UASSERT_NO_THROW(proc.Notify(os_signals::kSigUsr1));
  UASSERT_NO_THROW(proc.Notify(os_signals::kSigUsr2));

  Helper h1;
  Helper h2;

  const auto subscribe1 = proc.Get().AddListener(
      &h1, "h1", os_signals::kSigUsr1, &Helper::OnSigUsr1);
  const auto subscribe2 = proc.Get().AddListener(&h2, "h2", &Helper::OnSigUsrX);
  EXPECT_EQ(h1.sigusr1, 0);
  EXPECT_EQ(h1.sigusr2, 0);
  EXPECT_EQ(h2.sigusr1, 0);
  EXPECT_EQ(h2.sigusr2, 0);

  proc.Notify(os_signals::kSigUsr1);
  EXPECT_EQ(h1.sigusr1, 1);
  EXPECT_EQ(h1.sigusr2, 0);
  EXPECT_EQ(h2.sigusr1, 1);
  EXPECT_EQ(h2.sigusr2, 0);

  proc.Notify(os_signals::kSigUsr1);
  EXPECT_EQ(h1.sigusr1, 2);
  EXPECT_EQ(h1.sigusr2, 0);
  EXPECT_EQ(h2.sigusr1, 2);
  EXPECT_EQ(h2.sigusr2, 0);

  proc.Notify(os_signals::kSigUsr2);
  EXPECT_EQ(h1.sigusr1, 2);
  EXPECT_EQ(h1.sigusr2, 0);  // h1 is not subscribed to os_signals::kSigUsr2
  EXPECT_EQ(h2.sigusr1, 2);
  EXPECT_EQ(h2.sigusr2, 1);

  proc.Notify(os_signals::kSigUsr2);
  EXPECT_EQ(h1.sigusr1, 2);
  EXPECT_EQ(h1.sigusr2, 0);  // h1 is not subscribed to os_signals::kSigUsr2
  EXPECT_EQ(h2.sigusr1, 2);
  EXPECT_EQ(h2.sigusr2, 2);

  proc.Notify(os_signals::kSigUsr1);
  EXPECT_EQ(h1.sigusr1, 3);
  EXPECT_EQ(h1.sigusr2, 0);
  EXPECT_EQ(h2.sigusr1, 3);
  EXPECT_EQ(h2.sigusr2, 2);
}

USERVER_NAMESPACE_END
