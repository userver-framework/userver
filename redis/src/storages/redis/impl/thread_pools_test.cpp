#include <storages/redis/impl/thread_pools.hpp>

#include <memory>
#include <utility>

#include <userver/engine/async.hpp>
#include <userver/utest/utest.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::redis::impl {

UTEST_MT(ThreadPools, DestructionDoesNotBlockTaskProcessor, 1) {
    auto thread_pools = std::make_unique<ThreadPools>(1, 1);
    auto redis_thread_pool = thread_pools->GetRedisThreadPool();

    auto release_pool = engine::AsyncNoTracing([redis_thread_pool = std::move(redis_thread_pool)] {});

    thread_pools.reset();
    release_pool.Get();
}

}  // namespace storages::redis::impl

USERVER_NAMESPACE_END
