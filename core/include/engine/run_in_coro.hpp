#pragma once

/// @file engine/run_in_coro.hpp
/// @brief @copybrief RunInCoro

#include <cstddef>
#include <functional>

/// Deprecated, use engine::RunStandalone instead
void RunInCoro(std::function<void()> payload, std::size_t worker_threads = 1);
