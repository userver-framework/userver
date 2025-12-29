#pragma once

#include <moodycamel/blockingconcurrentqueue.h>
#include <moodycamel/lightweightsemaphore.h>
#include <boost/container/small_vector.hpp>
#include <boost/smart_ptr/intrusive_ptr.hpp>

#include <userver/utils/fixed_array.hpp>

#include <concurrent/impl/wait_wake.hpp>
#include <engine/task/task_processor_config.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine {

namespace impl {
class TaskContext;
}  // namespace impl

class TaskQueuePullPin final {
public:
    TaskQueuePullPin() = delete;
    TaskQueuePullPin(const TaskQueuePullPin&) = delete;
    TaskQueuePullPin(TaskQueuePullPin&&) = delete;
    TaskQueuePullPin& operator=(const TaskQueuePullPin&) = delete;
    TaskQueuePullPin& operator=(TaskQueuePullPin&&) = delete;

    explicit TaskQueuePullPin(const TaskProcessorConfig& config);

    void Push(boost::intrusive_ptr<impl::TaskContext>&& context);

    // Returns nullptr as a stop signal
    boost::intrusive_ptr<impl::TaskContext> PopBlocking();

    void StopProcessing();

    std::size_t GetSizeApproximate() const noexcept;

    void PrepareWorker(std::size_t index);

private:
    void DoPush(impl::TaskContext* context);

    impl::TaskContext* DoPopBlocking();

    using ContextPtr = impl::TaskContext*;
    using ContextQueue = moodycamel::ConcurrentQueue<ContextPtr>;

    struct GlobalQueue {
        ContextQueue queue{};
        std::atomic<std::size_t> size{0};
    };
    GlobalQueue global_queue_;

    class ThreadPinnedQueue {
    public:
        explicit ThreadPinnedQueue(GlobalQueue& global)
            : global_queue_consumer_token_{global.queue}
        {}

        std::size_t PushAndReturnSize(ContextPtr context);
        bool TryExtract(GlobalQueue& global, ContextPtr& context_out);
        void StartNewContextRetrieval() noexcept;
        std::size_t GetSizeApproximate() const noexcept;

    private:
        moodycamel::ConcurrentQueue<impl::TaskContext*> queue_{};

        // While not going into sleep, the size_ is not 0. This allows the DoPush to do less wakeups via syscalls.
        std::atomic<std::size_t> size_{1};

        moodycamel::ConsumerToken consumer_token_{queue_};
        moodycamel::ConsumerToken global_queue_consumer_token_;

        boost::container::small_vector<ContextPtr, 16> unsync_queue_;

        std::size_t local_queue_fetch_counter_{0};
        bool size_was_decremented_ = true;

        void DecrementSizeIfWasNot() noexcept;
        void IncrementSizeIfNeeded() noexcept;

        bool TryExtractFromLocal(ContextPtr& out_context);
        bool TryExtractFromUnsyncQueue(ContextPtr& out_context);
        std::size_t MoveToUnsyncQueue();
        bool TryExtractFromGlobalQueue(GlobalQueue& global, ContextPtr& out_context);
    };

    utils::FixedArray<ThreadPinnedQueue> thread_pined_queue_;

    concurrent::impl::WaitWake wait_wake_;
};

}  // namespace engine

USERVER_NAMESPACE_END
