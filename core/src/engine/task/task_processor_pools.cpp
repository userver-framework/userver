#include <engine/task/task_processor_pools.hpp>

#include <engine/task/task_context.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::impl {

TaskProcessorPools::TaskProcessorPools(coro::PoolConfig coro_pool_config,
                                       ev::ThreadPoolConfig ev_pool_config)
    : coro_pool_(std::move(coro_pool_config), &TaskContext::CoroFunc),
      event_thread_pool_(std::move(ev_pool_config),
                         ev::ThreadPool::kUseDefaultEvLoop) {}

}  // namespace engine::impl

USERVER_NAMESPACE_END
