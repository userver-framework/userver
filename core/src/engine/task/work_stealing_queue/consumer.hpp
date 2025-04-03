#pragma once

#include <array>
#include <condition_variable>
#include <cstddef>
#include <random>

#include <engine/task/work_stealing_queue/global_queue.hpp>
#include <engine/task/work_stealing_queue/local_queue.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine {

namespace impl {
class TaskContext;
}  // namespace impl

class WorkStealingTaskQueue;
class ConsumersManager;

class Consumer final {
public:
    Consumer(WorkStealingTaskQueue& owner, ConsumersManager& consumers_manager);

    void Push(impl::TaskContext* ctx);

    impl::TaskContext* PopBlocking();

    std::size_t GetLocalQueueSize() const noexcept;

    WorkStealingTaskQueue* GetOwner() const noexcept;

private:
    friend ConsumersManager;
    friend WorkStealingTaskQueue;

    void SetIndex(std::size_t index) noexcept;

    bool IsStopped() const noexcept;

    void EmptySurplusQueue(impl::TaskContext* extra);

    impl::TaskContext* StealFromAnotherConsumerOrGlobalQueue(const std::size_t attempts, std::size_t to_steal);

    std::size_t Steal(utils::span<impl::TaskContext*> buffer);

    impl::TaskContext* TryPopFromOwnerQueue(const bool is_global);

    impl::TaskContext* ProbabilisticPopFromOwnerQueues();

    impl::TaskContext* TryPop();

    impl::TaskContext* TryPopBeforeSleep();

    impl::TaskContext* TryPopLocal();

    impl::TaskContext* DoPop();

    void Sleep(const std::int32_t old_sleep_counter);

    void WakeUp();

    // Empirical evaluation of the optimal local queue size.
    // 2^n -1 so that the internal queue size is 2^n
    static constexpr std::size_t kLocalQueueSize = 1024;
    // Equal to half the queue size
    static constexpr std::size_t kConsumerStealBufferSize = 512;

    LocalQueue<impl::TaskContext, kLocalQueueSize> local_queue_{};
    LocalQueue<impl::TaskContext, kConsumerStealBufferSize> local_queue_surplus_{};
    WorkStealingTaskQueue& owner_;
    ConsumersManager& consumers_manager_;
    const std::size_t steal_attempts_count_;
    std::size_t inner_index_{0};
    // kConsumerStealBufferSize + 1 for extra task in push
    std::array<impl::TaskContext*, kConsumerStealBufferSize + 1> steal_buffer_{};
    std::minstd_rand rnd_;
    std::size_t steps_count_{0};
    std::atomic<std::int32_t> sleep_counter_{0};
    GlobalQueue::Token global_queue_token_;
    GlobalQueue::Token background_queue_token_;
#ifndef __linux__
    std::condition_variable cv_;
    std::mutex mutex_;
#endif
};

}  // namespace engine
USERVER_NAMESPACE_END
