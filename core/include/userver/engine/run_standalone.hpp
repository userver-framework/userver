#pragma once

/// @file userver/engine/run_standalone.hpp
/// @brief @copybrief engine::RunStandalone

#include <cstddef>
#include <functional>
#include <string>

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

struct TaskProcessorPoolsConfig {
  std::size_t worker_threads = 1;
  std::size_t initial_coro_pool_size = 10;
  std::size_t max_coro_pool_size = 100;
  std::size_t ev_threads_num = 1;
  std::string ev_thread_name = "ev";
};

void RunStandalone(const TaskProcessorPoolsConfig& config,
                   std::function<void()> payload);
/// @}

}  // namespace engine
