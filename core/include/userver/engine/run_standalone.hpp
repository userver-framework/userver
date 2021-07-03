#pragma once

/// @file engine/run_standalone.hpp
/// @brief @copybrief engine::RunStandalone

#include <cstddef>
#include <functional>

namespace engine {

/// @{
/// @brief Runs a payload in a temporary coroutine engine instance.
///
/// Creates a TaskProcessor with specified parameters, executes the payload
/// in an Async and exits. Mainly designated for async code unit testing.
///
/// @warning This function must not be used while another engine instance is
/// running.
///
/// @param payload Code to be run in a Task
/// @param worker_threads Engine thread pool size, 1 by default
void RunStandalone(std::function<void()> payload);

void RunStandalone(std::size_t worker_threads, std::function<void()> payload);
/// @}

}  // namespace engine
