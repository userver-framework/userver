#pragma once

#include <memory>

USERVER_NAMESPACE_BEGIN

namespace engine::ev {
class ThreadPool;
}  // namespace engine::ev

namespace redis {

class ThreadPools {
 public:
  ThreadPools(size_t sentinel_thread_pool_size, size_t redis_thread_pool_size);
  ~ThreadPools();

  engine::ev::ThreadPool& GetSentinelThreadPool() const;
  const std::shared_ptr<engine::ev::ThreadPool>& GetRedisThreadPool() const;

 private:
  // Sentinel and Redis should use separate thread pools to avoid deadlocks.
  // Sentinel waits synchronously while Redis starts and stops watchers in
  // Connect()/Disconnect().
  std::unique_ptr<engine::ev::ThreadPool> sentinel_thread_pool_;
  std::shared_ptr<engine::ev::ThreadPool> redis_thread_pool_;
};

}  // namespace redis

USERVER_NAMESPACE_END
