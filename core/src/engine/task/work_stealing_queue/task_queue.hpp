#pragma once

#include <cstddef>

#include <boost/smart_ptr/intrusive_ptr.hpp>

#include <engine/task/task_processor_config.hpp>
#include <engine/task/work_stealing_queue/consumer.hpp>
#include <engine/task/work_stealing_queue/consumers_manager.hpp>
#include <engine/task/work_stealing_queue/global_queue.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine {

namespace impl {
class TaskContext;
}  // namespace impl

class WorkStealingTaskQueue final {
    friend class Consumer;

public:
    explicit WorkStealingTaskQueue(const TaskProcessorConfig& config);

    void Push(boost::intrusive_ptr<impl::TaskContext>&& context);
    // Returns nullptr as a stop signal
    boost::intrusive_ptr<impl::TaskContext> PopBlocking();

    void StopProcessing();

    std::size_t GetSizeApproximate() const noexcept;

    void PrepareWorker(std::size_t index);

private:
    void DoPush(impl::TaskContext* context);

    impl::TaskContext* DoPopBlocking();

    Consumer* GetConsumer();

    const std::size_t consumers_count_;

    GlobalQueue global_queue_;
    GlobalQueue background_queue_;
    utils::FixedArray<Consumer> consumers_;
    ConsumersManager consumers_manager_;
};

}  // namespace engine

USERVER_NAMESPACE_END
