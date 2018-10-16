#pragma once

#include <gtest/gtest.h>
#include <functional>

void RunInCoro(std::function<void()>, size_t worker_threads = 1);

inline void TestInCoro(std::function<void()> callback,
                       size_t worker_threads = 1) {
  RunInCoro(std::move(callback), worker_threads);
}
