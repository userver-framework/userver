#include <storages/redis/impl/thread_pools.hpp>

#include <chrono>
#include <cstddef>
#include <string>
#include <thread>

#include <userver/engine/sleep.hpp>
#include <userver/engine/task/current_task.hpp>
#include <userver/logging/log.hpp>
#include <userver/utils/assert.hpp>

#include <engine/ev/thread_pool.hpp>
#include <engine/ev/thread_pool_config.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

const std::chrono::milliseconds kThreadPoolWaitingSleepTime{20};

const std::string kSentinelThreadName = "redis_sentinel";
const std::string kRedisThreadName = "redis_client";

void SynchronizeThreadPool(engine::ev::ThreadPool& pool) {
    for (std::size_t thread_idx = 0; thread_idx < pool.GetSize(); ++thread_idx) {
        pool.NextThread().RunInEvLoopSync([] {});
    }
}

void SleepWhileWaitingForThreadPool() {
    if (engine::current_task::IsTaskProcessorThread()) {
        engine::SleepFor(kThreadPoolWaitingSleepTime);
    } else {
        std::this_thread::sleep_for(kThreadPoolWaitingSleepTime);
    }
}

}  // namespace

namespace storages::redis::impl {

ThreadPools::ThreadPools(size_t sentinel_thread_pool_size, size_t redis_thread_pool_size) {
    sentinel_thread_pool_ = std::make_unique<
        engine::ev::ThreadPool>(engine::ev::ThreadPoolConfig{sentinel_thread_pool_size, kSentinelThreadName});

    redis_thread_pool_ = std::make_shared<
        engine::ev::ThreadPool>(engine::ev::ThreadPoolConfig{redis_thread_pool_size, kRedisThreadName});
}

ThreadPools::~ThreadPools() {
    LOG_INFO() << "Stopping redis thread pools";
    while (redis_thread_pool_.use_count() > 1) {
        SleepWhileWaitingForThreadPool();
    }
    LOG_INFO() << "Stopped redis thread pools";
}

engine::ev::ThreadPool& ThreadPools::GetSentinelThreadPool() const { return *sentinel_thread_pool_; }

const std::shared_ptr<engine::ev::ThreadPool>& ThreadPools::GetRedisThreadPool() const { return redis_thread_pool_; }

void ThreadPools::SynchronizeEventLoops() const {
    UASSERT(engine::current_task::IsTaskProcessorThread());
    SynchronizeThreadPool(*sentinel_thread_pool_);
    SynchronizeThreadPool(*redis_thread_pool_);
}

}  // namespace storages::redis::impl

USERVER_NAMESPACE_END
