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

}  // namespace

USERVER_NAMESPACE_END
