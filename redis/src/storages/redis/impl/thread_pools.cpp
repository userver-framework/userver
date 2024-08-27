#include <userver/storages/redis/impl/thread_pools.hpp>

#include <chrono>
#include <thread>

#include <userver/logging/log.hpp>

#include <engine/ev/thread_pool.hpp>
#include <engine/ev/thread_pool_config.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

const std::chrono::milliseconds kThreadPoolWaitingSleepTime{20};

const std::string kSentinelThreadName = "redis_sentinel";
const std::string kRedisThreadName = "redis_client";

}  // namespace

namespace redis {

ThreadPools::ThreadPools(size_t sentinel_thread_pool_size,
                         size_t redis_thread_pool_size) {
  sentinel_thread_pool_ =
      std::make_unique<engine::ev::ThreadPool>(engine::ev::ThreadPoolConfig{
          sentinel_thread_pool_size, kSentinelThreadName});

  redis_thread_pool_ = std::make_shared<engine::ev::ThreadPool>(
      engine::ev::ThreadPoolConfig{redis_thread_pool_size, kRedisThreadName});
}

ThreadPools::~ThreadPools() {
  LOG_INFO() << "Stopping redis thread pools";
  while (redis_thread_pool_.use_count() > 1) {
    std::this_thread::sleep_for(kThreadPoolWaitingSleepTime);
  }
  LOG_INFO() << "Stopped redis thread pools";
}

engine::ev::ThreadPool& ThreadPools::GetSentinelThreadPool() const {
  return *sentinel_thread_pool_;
}

const std::shared_ptr<engine::ev::ThreadPool>& ThreadPools::GetRedisThreadPool()
    const {
  return redis_thread_pool_;
}

}  // namespace redis

USERVER_NAMESPACE_END
