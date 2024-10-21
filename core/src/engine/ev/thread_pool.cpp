#include "thread_pool.hpp"

#include <fmt/format.h>

#include <userver/utils/assert.hpp>

#include "thread.hpp"
#include "thread_control.hpp"

USERVER_NAMESPACE_BEGIN

namespace engine::ev {

ThreadPool::ThreadPool(ThreadPoolConfig config) : ThreadPool(std::move(config), false) {}

ThreadPool::ThreadPool(ThreadPoolConfig config, UseDefaultEvLoop)
    : ThreadPool(std::move(config), !config.ev_default_loop_disabled) {}

ThreadPool::ThreadPool(ThreadPoolConfig config, bool use_ev_default_loop) : use_ev_default_loop_(use_ev_default_loop) {
    threads_ = utils::GenerateFixedArray(config.threads, [&](std::size_t index) {
        const auto thread_name = fmt::format("{}_{}", config.thread_name, index);
        return (use_ev_default_loop && index == 0) ? Thread(thread_name, Thread::kUseDefaultEvLoop)
                                                   : Thread(thread_name);
    });

    default_controls_.controls = utils::GenerateFixedArray(threads_.size(), [this](std::size_t index) {
        return ThreadControl(threads_[index]);
    });

    timer_controls_.controls = utils::GenerateFixedArray(threads_.size(), [this](std::size_t index) {
        return TimerThreadControl{threads_[index]};
    });
}

ThreadPool::~ThreadPool() {
    // DATA RACE if not stopping the threads first.
    // Pointer to TimerThreadControl could be in execution queue of thread or
    // waiting for ev loop event. Stopping the threads makes sure that no pointer
    // to *Control is pending.
    threads_ = {};
}

std::size_t ThreadPool::GetSize() const { return threads_.size(); }

ThreadControl& ThreadPool::NextThread() { return default_controls_.Next(); }

TimerThreadControl& ThreadPool::NextTimerThread() { return timer_controls_.Next(); }

ThreadControl& ThreadPool::GetEvDefaultLoopThread() {
    UINVARIANT(!default_controls_.Empty() && use_ev_default_loop_, "no ev_default_loop in current thread_pool");
    return default_controls_.controls[0];
}

void ThreadPool::WriteStats(utils::statistics::Writer& writer) const {
    for (auto& thread : threads_) {
        writer.ValueWithLabels(thread.GetCurrentLoadPercent(), {{"ev_thread_name", thread.GetName()}});
    }
}

}  // namespace engine::ev

USERVER_NAMESPACE_END
