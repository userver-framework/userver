#include <userver/tracing/scope_time.hpp>

#include <userver/engine/sleep.hpp>

#include <userver/utest/utest.hpp>

USERVER_NAMESPACE_BEGIN

TEST(ScopeTime, Durations) {
  using Duration = tracing::ScopeTime::Duration;
  using DurationMillis = tracing::ScopeTime::DurationMillis;
  using namespace std::literals::chrono_literals;

  Duration d = 42us;
  EXPECT_NE(std::chrono::duration_cast<DurationMillis>(d), DurationMillis{});
  EXPECT_GE(std::chrono::duration_cast<DurationMillis>(d), 40us);
  EXPECT_LE(std::chrono::duration_cast<DurationMillis>(d), 44us);
}

UTEST(ScopeTime, Constructor) {
  tracing::ScopeTime sc;
  tracing::ScopeTime sc2("da name");

  engine::SleepFor(std::chrono::milliseconds(1));

  EXPECT_GE(sc2.DurationSinceReset(), std::chrono::milliseconds(1));
  EXPECT_EQ(sc.DurationSinceReset(), std::chrono::milliseconds(0));

  EXPECT_GE(sc2.ElapsedSinceReset(), std::chrono::milliseconds(1));
  EXPECT_EQ(sc.ElapsedSinceReset(), std::chrono::milliseconds(0));
}

USERVER_NAMESPACE_END
