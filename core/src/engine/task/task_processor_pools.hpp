#pragma once

#include <engine/coro/pool.hpp>
#include <engine/ev/thread_pool.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::impl {

class TaskContext;

class TaskProcessorPools final {
 public:
  using CoroPool = coro::Pool<TaskContext>;

  TaskProcessorPools(coro::PoolConfig coro_pool_config,
                     ev::ThreadPoolConfig ev_pool_config);

  CoroPool& GetCoroPool() { return coro_pool_; }
  ev::ThreadPool& EventThreadPool() { return event_thread_pool_; }

 private:
  CoroPool coro_pool_;
  ev::ThreadPool event_thread_pool_;
};

}  // namespace engine::impl

USERVER_NAMESPACE_END
