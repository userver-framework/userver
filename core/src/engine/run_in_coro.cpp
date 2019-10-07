#include <atomic>
#include <chrono>
#include <condition_variable>
#include <exception>
#include <mutex>

#include <engine/standalone.hpp>

void RunInCoro(std::function<void()> user_cb, size_t worker_threads) {
  auto task_processor_holder =
      engine::impl::TaskProcessorHolder::MakeTaskProcessor(
          worker_threads, "task_processor",
          engine::impl::MakeTaskProcessorPools());

  engine::impl::RunOnTaskProcessorSync(*task_processor_holder, user_cb);
}
