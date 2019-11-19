#pragma once

#include <gtest/gtest.h>

#include <chrono>
#include <functional>

void RunInCoro(std::function<void()>, size_t worker_threads = 1);

inline void TestInCoro(std::function<void()> callback,
                       size_t worker_threads = 1) {
  RunInCoro(std::move(callback), worker_threads);
}

inline constexpr std::chrono::seconds kMaxTestWaitTime(20);

namespace std::chrono {

inline std::ostream& operator<<(std::ostream& os, std::chrono::seconds s) {
  return os << s.count() << "s";
}

inline std::ostream& operator<<(std::ostream& os,
                                std::chrono::milliseconds ms) {
  return os << ms.count() << "ms";
}

inline std::ostream& operator<<(std::ostream& os,
                                std::chrono::microseconds us) {
  return os << us.count() << "us";
}

inline std::ostream& operator<<(std::ostream& os, std::chrono::nanoseconds ns) {
  return os << ns.count() << "ns";
}
}  // namespace std::chrono

namespace formats::json {
class Value;
// for gtest-only
std::ostream& operator<<(std::ostream&, const Value&);
};  // namespace formats::json

#ifdef __APPLE__
#define DISABLED_IN_MAC_OS_TEST_NAME(name) DISABLED_##name
#else
#define DISABLED_IN_MAC_OS_TEST_NAME(name) name
#endif

#ifdef _LIBCPP_VERSION
#define DISABLED_IN_LIBCPP_TEST_NAME(name) DISABLED_##name
#else
#define DISABLED_IN_LIBCPP_TEST_NAME(name) name
#endif
