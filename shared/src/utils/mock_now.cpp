#include <utils/mock_now.hpp>

#include <atomic>
#include <mutex>

#ifdef MOCK_NOW

namespace utils::datetime {

namespace {

// There's no difference between std::memory_order_seq_cst and
// std::memory_order_relaxed reads on x86 https://godbolt.org/g/ha4Yga
//
// Atomic stores are used only in tests.
std::atomic<bool> now_set{false};

// GCC-4.8 fails to store system_clock::time_point in std::atomic,
// so we are using system_clock::time_point::duration.
std::atomic<std::chrono::system_clock::time_point::duration> now{};

}  // namespace

std::chrono::system_clock::time_point MockNow() {
  if (!now_set) return std::chrono::system_clock::now();

  return std::chrono::system_clock::time_point{now.load()};
}

std::chrono::steady_clock::time_point MockSteadyNow() {
  if (!now_set) return std::chrono::steady_clock::now();

  return std::chrono::steady_clock::time_point{now.load()};
}

void MockNowSet(std::chrono::system_clock::time_point point) {
  now = point.time_since_epoch();
  now_set = true;
}

void MockSleep(std::chrono::seconds sec) { now = now.load() + sec; }

void MockSleep(std::chrono::milliseconds msec) { now = now.load() + msec; }

void MockNowUnset() { now_set = false; }

bool IsMockNow() { return now_set; }

}  // namespace utils::datetime

#endif  // MOCK_NOW
