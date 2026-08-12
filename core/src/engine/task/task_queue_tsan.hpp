#pragma once

#include <moodycamel/blockingconcurrentqueue.h>
#include <moodycamel/lightweightsemaphore.h>
#include <boost/smart_ptr/intrusive_ptr.hpp>

#include <userver/utils/fixed_array.hpp>

#include <engine/task/task_processor_config.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine {

namespace impl {
class TaskContext;
}  // namespace impl

class TaskQueueTSan final {
public:
    explicit TaskQueueTSan(const TaskProcessorConfig& config);

    void Push(boost::intrusive_ptr<impl::TaskContext>&& context);

    // Returns nullptr as a stop signal
    boost::intrusive_ptr<impl::TaskContext> PopBlocking();

    void StopProcessing();

    std::size_t GetSizeApproximate() const noexcept;

    void PrepareWorker(std::size_t index);

private:
    std::size_t GetBestThreadForNewTask() const noexcept;

    void DoPush(impl::TaskContext* context);

    void DoPushIntoThread(impl::TaskContext* context, std::size_t thread_index);

    impl::TaskContext* DoPopBlocking();

    struct ThreadTSanItem {
        moodycamel::LightweightSemaphore semaphore;
        moodycamel::ConcurrentQueue<impl::TaskContext*> queue{};
        moodycamel::ConsumerToken token{queue};
    };
    utils::FixedArray<ThreadTSanItem> thread_pined_;

    mutable std::atomic<std::size_t> round_robin_index_{0};
};

}  // namespace engine

USERVER_NAMESPACE_END
