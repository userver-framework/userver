#pragma once

#include <cstddef>

#include <boost/smart_ptr/intrusive_ptr.hpp>

#include <engine/task/task_processor_config.hpp>
#include <engine/task/work_stealing_queue/coordinator.hpp>
#include <engine/task/work_stealing_queue/global_queue.hpp>
#include <engine/task/work_stealing_queue/worker.hpp>
#include <userver/utils/fixed_array.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::fast {

class Queue final {
public:
    explicit Queue(const TaskProcessorConfig& config);

    void Push(boost::intrusive_ptr<impl::TaskContext>&& context);

    boost::intrusive_ptr<impl::TaskContext> PopBlocking();

    void StopProcessing() noexcept;

    std::size_t GetSizeApproximate() const noexcept;

    void PrepareWorker(std::size_t index) noexcept;

private:
    friend class Worker;

    void DoPush(impl::TaskContext* context);

    Worker* GetLocalWorker() const noexcept;

    const std::size_t workers_count_;

    GlobalIntrusiveQueue global_queue_;
    Coordinator coordinator_;
    utils::FixedArray<Worker> workers_;
};

}  // namespace engine::fast

USERVER_NAMESPACE_END
