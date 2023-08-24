#include <userver/utils/mock_now.hpp>

#include <atomic>

#include <userver/utils/assert.hpp>
#include <userver/utils/atomic.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::datetime {

namespace {

constexpr auto kNotMocked = std::chrono::system_clock::time_point::min();

// There's no difference between std::memory_order_seq_cst and
// std::memory_order_relaxed reads on x86 https://godbolt.org/g/ha4Yga
//
// Atomic stores are used only in tests.
std::atomic<std::chrono::system_clock::time_point> now{kNotMocked};

}  // namespace

std::chrono::system_clock::time_point MockNow() noexcept {
  const auto mocked_now_value = now.load();
  return mocked_now_value == kNotMocked ? std::chrono::system_clock::now()
                                        : mocked_now_value;
}

std::chrono::steady_clock::time_point MockSteadyNow() noexcept {
  const auto mocked_now_value = now.load();
  return mocked_now_value == kNotMocked
             ? std::chrono::steady_clock::now()
             : std::chrono::steady_clock::time_point{
                   mocked_now_value.time_since_epoch()};
}

void MockNowSet(std::chrono::system_clock::time_point new_mocked_now) {
  UINVARIANT(new_mocked_now != kNotMocked,
             "This mocked time value is reserved, "
             "use MockNowUnset() instead");
  now = new_mocked_now;
}

void MockSleep(std::chrono::seconds duration) {
  MockSleep(std::chrono::duration_cast<std::chrono::milliseconds>(duration));
}

void MockSleep(std::chrono::milliseconds duration) {
  utils::AtomicUpdate(now, [duration](auto old) {
    UINVARIANT(old != kNotMocked,
               "Set the initial mocked time using MockNowSet before MockSleep");
    return old + duration;
  });
}

void MockNowUnset() { now = kNotMocked; }

bool IsMockNow() { return now.load() != kNotMocked; }

}  // namespace utils::datetime

USERVER_NAMESPACE_END
