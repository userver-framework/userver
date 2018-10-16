#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>

#include <engine/async.hpp>
#include <engine/standalone.hpp>

void RunInCoro(std::function<void()> user_cb, size_t worker_threads) {
  auto task_processor_holder =
      engine::impl::TaskProcessorHolder::MakeTaskProcessor(
          worker_threads, "task_processor",
          engine::impl::MakeTaskProcessorPools());

  std::mutex mutex;
  std::condition_variable cv;
  std::atomic_bool done{false};

  auto cb = [&user_cb, &mutex, &done, &cv]() {
    user_cb();

    std::lock_guard<std::mutex> lock(mutex);
    done = true;
    cv.notify_all();
  };
  engine::Async(*task_processor_holder, std::move(cb)).Detach();

  std::unique_lock<std::mutex> lock(mutex);
  cv.wait(lock, [&done]() { return done.load(); });
}
