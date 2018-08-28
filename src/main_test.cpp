#include <gtest/gtest.h>
#include <engine/async.hpp>
#include <engine/task/task_context.hpp>

void TestInCoro(std::function<void()> user_cb, size_t worker_threads) {
  engine::ev::Thread thread("test_thread");
  engine::ev::ThreadControl thread_control(thread);

  engine::coro::PoolConfig pool_config;
  engine::ev::ThreadPool thread_pool(1, "pool");
  engine::TaskProcessor::CoroPool coro_pool(
      pool_config, &engine::impl::TaskContext::CoroFunc);
  engine::TaskProcessorConfig tp_config;
  tp_config.worker_threads = worker_threads;
  tp_config.thread_name = "task_processor";
  engine::TaskProcessor task_processor(tp_config, coro_pool, thread_pool);

  std::mutex mutex;
  std::condition_variable cv;
  std::atomic_bool done{false};

  auto cb = [&user_cb, &mutex, &done, &cv]() {
    user_cb();

    std::lock_guard<std::mutex> lock(mutex);
    done = true;
    cv.notify_all();
  };
  engine::Async(task_processor, std::move(cb)).Detach();

  auto timeout = std::chrono::seconds(10);
  {
    std::unique_lock<std::mutex> lock(mutex);
    cv.wait_for(lock, timeout, [&done]() { return done.load(); });
  }
  EXPECT_EQ(true, done.load());
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);

  return RUN_ALL_TESTS();
}
