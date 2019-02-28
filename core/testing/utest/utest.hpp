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
