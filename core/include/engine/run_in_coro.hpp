#pragma once

/// @file engine/run_in_coro.hpp
/// @brief @copybrief RunInCoro

#include <functional>
#include <optional>

/// @brief Runs a payload in a temporary coroutine engine instance.
///
/// Creates a TaskProcessor with specified parameters, executes the payload
/// in an Async and exits. Mainly designated for async code unit testing.
///
/// @warning This function must not be used while another engine instance is
/// running.
///
/// @param payload Code to be run in a Task
/// @param worker_threads Engine thread pool size
/// @param initial_coro_pool_size Number of preallocated coroutines
/// @param max_coro_pool_size Maximum number of simultaneously allocated tasks
void RunInCoro(std::function<void()> payload, size_t worker_threads = 1,
               std::optional<size_t> initial_coro_pool_size = {},
               std::optional<size_t> max_coro_pool_size = {});
