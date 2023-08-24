#include <engine/task/task_processor_pools.hpp>

#include <utility>

#include <engine/task/task_context.hpp>
#include <userver/logging/log.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::impl {

TaskProcessorPools::TaskProcessorPools(coro::PoolConfig coro_pool_config,
                                       ev::ThreadPoolConfig ev_pool_config)
    : coro_pool_(std::move(coro_pool_config), &TaskContext::CoroFunc),
      event_thread_pool_(std::move(ev_pool_config),
                         ev::ThreadPool::kUseDefaultEvLoop) {
  const bool old_value =
      std::exchange(logging::impl::has_background_threads_which_can_log, true);
  UASSERT_MSG(!old_value,
              "Launching multiple unrelated coroutine engines can result in "
              "random lockups or performance degradation");
}

TaskProcessorPools::~TaskProcessorPools() {
  const bool old_value =
      std::exchange(logging::impl::has_background_threads_which_can_log, false);
  UASSERT(old_value);
}

}  // namespace engine::impl

USERVER_NAMESPACE_END
