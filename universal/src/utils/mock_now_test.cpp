#include <gtest/gtest.h>

#include <userver/utils/datetime.hpp>
#include <userver/utils/mock_now.hpp>

USERVER_NAMESPACE_BEGIN

using namespace std::chrono_literals;

namespace {

/// [Mocked time sample]
class Timer final {
 public:
  // Starts the next loop and returns time elapsed on the previous loop
  std::chrono::system_clock::duration NextLoop() {
    const auto now = utils::datetime::Now();
    const auto elapsed = now - loop_start_;
    loop_start_ = now;
    return elapsed;
  }

 private:
  std::chrono::system_clock::time_point loop_start_{utils::datetime::Now()};
};

TEST(MockNow, Timer) {
  using utils::datetime::Stringtime;

  utils::datetime::MockNowSet(Stringtime("2000-01-01T00:00:00+0000"));
  Timer timer;

  utils::datetime::MockSleep(9001s);  // does NOT sleep, just sets the new time
  EXPECT_EQ(timer.NextLoop(), 9001s);

  utils::datetime::MockNowSet(Stringtime("2000-01-02T00:00:00+0000"));
  EXPECT_EQ(timer.NextLoop(), 24h - 9001s);
}
/// [Mocked time sample]

TEST(MockSteadyNow, StdStdSeqAlwaysValid) {
  EXPECT_NO_THROW({
    utils::datetime::MockNowUnset();
    [[maybe_unused]] const auto prev_now = utils::datetime::MockSteadyNow();
    [[maybe_unused]] const auto curr_now = utils::datetime::MockSteadyNow();
  });
}

TEST(MockSteadyNow, StdMockedSeqAlwaysValid) {
  using utils::datetime::Stringtime;

  EXPECT_NO_THROW({
    utils::datetime::MockNowUnset();
    [[maybe_unused]] const auto prev_value = utils::datetime::MockSteadyNow();
    utils::datetime::MockNowSet(Stringtime("1900-01-01T00:00:00+0000"));
    [[maybe_unused]] const auto current_value =
        utils::datetime::MockSteadyNow();
  });
}

TEST(MockSteadyNow, MockedStdSeqAlwaysValid) {
  using utils::datetime::Stringtime;

  EXPECT_NO_THROW({
    utils::datetime::MockNowSet(Stringtime("2050-01-01T00:00:00+0000"));
    [[maybe_unused]] const auto prev_value = utils::datetime::MockSteadyNow();
    utils::datetime::MockNowUnset();
    [[maybe_unused]] const auto current_value =
        utils::datetime::MockSteadyNow();
  });
}

TEST(MockSteadyNow, MockedValidSeq) {
  using utils::datetime::Stringtime;

  EXPECT_NO_THROW({
    utils::datetime::MockNowSet(Stringtime("2000-01-01T00:00:00+0000"));
    [[maybe_unused]] const auto prev_value = utils::datetime::MockSteadyNow();
    utils::datetime::MockNowSet(Stringtime("2000-01-02T00:00:00+0000"));
    [[maybe_unused]] const auto current_value =
        utils::datetime::MockSteadyNow();
  });
}

}  // namespace

USERVER_NAMESPACE_END
