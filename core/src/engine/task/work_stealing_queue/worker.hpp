#pragma once

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <random>
#include <span>

#include <boost/intrusive/list.hpp>

#include <engine/task/work_stealing_queue/local_queue.hpp>
#include <userver/utils/impl/intrusive_link_mode.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::fast {

class Coordinator;
class Queue;

using WorkerIdleHook = boost::intrusive::list_base_hook<
    utils::impl::IntrusiveLinkMode,
    boost::intrusive::link_mode<boost::intrusive::safe_link>>;

class Worker final : public WorkerIdleHook {
public:
    Worker(Queue& owner, Coordinator& coordinator, std::size_t index, std::size_t workers_count) noexcept;

    void Push(impl::TaskContext* context);

    impl::TaskContext* PopBlocking();

    std::size_t StealTasks(std::span<impl::TaskContext*> out_buffer) noexcept;

    void Wake() noexcept;

    Queue* GetOwner() const noexcept;

    std::size_t GetLocalQueueSize() const noexcept;

private:
    friend class Coordinator;

    void OffloadToGlobal(impl::TaskContext* overflow) noexcept;

    impl::TaskContext* TryPopFromGlobalQueue() noexcept;

    impl::TaskContext* GrabTasksFromGlobalQueue() noexcept;

    impl::TaskContext* TryStealTasks(std::size_t series) noexcept;

    impl::TaskContext* TryPop() noexcept;

    impl::TaskContext* TryPopBeforeSleep() noexcept;

    void Sleep(std::int32_t old_sleep_counter) noexcept;

    Queue& owner_;
    Coordinator& coordinator_;
    const std::size_t index_;
    const std::size_t workers_count_;

    std::size_t iter_{0};

    SPMCQueue local_queue_;
    std::minstd_rand rnd_;
    std::array<impl::TaskContext*, kLocalQueueCapacity> steal_buffer_{};

    std::atomic<std::int32_t> sleep_counter_{0};
#ifndef __linux__
    std::condition_variable cv_;
    std::mutex mutex_;
#endif
};

}  // namespace engine::fast

USERVER_NAMESPACE_END
