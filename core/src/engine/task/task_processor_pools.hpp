#pragma once

#include <engine/coro/pool.hpp>
#include <engine/deadlock_detector.hpp>
#include <engine/ev/thread_pool.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::impl {

class TaskContext;

class TaskProcessorPools final {
public:
    using CoroPool = coro::Pool;

    TaskProcessorPools(coro::PoolConfig coro_pool_config, ev::ThreadPoolConfig ev_pool_config);

    ~TaskProcessorPools();

    CoroPool& GetCoroPool() { return coro_pool_; }
    ev::ThreadPool& EventThreadPool() { return event_thread_pool_; }
    engine::deadlock_detector::State& GetDeadlockDetectorState() { return dd_state_; }

private:
    CoroPool coro_pool_;
    ev::ThreadPool event_thread_pool_;
    engine::deadlock_detector::State dd_state_;
};

}  // namespace engine::impl

USERVER_NAMESPACE_END
