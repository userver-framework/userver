#pragma once

#include <gtest/gtest.h>
#include <functional>

void TestInCoro(std::function<void()>, size_t worker_threads = 1);
