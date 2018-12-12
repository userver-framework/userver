#include <atomic>
#include <chrono>
#include <condition_variable>
#include <exception>
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
  std::exception_ptr ex;

  auto cb = [&user_cb, &mutex, &done, &cv, &ex]() {
    try {
      user_cb();
    } catch (const std::exception&) {
      ex = std::current_exception();
    }

    std::lock_guard<std::mutex> lock(mutex);
    done = true;
    cv.notify_all();
  };
  engine::Async(*task_processor_holder, std::move(cb)).Detach();

  std::unique_lock<std::mutex> lock(mutex);
  cv.wait(lock, [&done]() { return done.load(); });
  if (ex) std::rethrow_exception(ex);
}
