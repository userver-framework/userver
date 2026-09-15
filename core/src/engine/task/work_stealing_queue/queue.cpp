#include <engine/task/work_stealing_queue/queue.hpp>

#include <userver/utils/assert.hpp>
#include <userver/utils/fixed_array.hpp>

#include <engine/task/task_context.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::fast {

namespace {

thread_local Worker* local_worker = nullptr;

}  // namespace

Queue::Queue(const TaskProcessorConfig& config)
    : workers_count_(config.worker_threads),
      coordinator_(config.worker_threads),
      workers_(utils::GenerateFixedArray(
          workers_count_,
          [this](std::size_t index) { return Worker{*this, coordinator_, index, workers_count_}; }
      ))
{}

void Queue::PrepareWorker(const std::size_t index) noexcept {
    UINVARIANT(index < workers_count_, "index must be less than workers count");
    local_worker = &workers_[index];
}

Worker* Queue::GetLocalWorker() const noexcept { return local_worker; }

void Queue::Push(boost::intrusive_ptr<impl::TaskContext>&& context) {
    UASSERT(context);
    DoPush(context.get());
    context.detach();
}

void Queue::DoPush(impl::TaskContext* const context) {
    UASSERT(context);

    if (context->IsBackground()) {
        global_queue_.Push(boost::intrusive_ptr<impl::TaskContext>{context, /* add_ref= */ false});
    } else {
        Worker* const worker = GetLocalWorker();

        if (worker != nullptr && worker->GetOwner() == this) {
            worker->Push(context);
        } else {
            global_queue_.Push(boost::intrusive_ptr<impl::TaskContext>{context, /* add_ref= */ false});
        }
    }

    coordinator_.NotifyNewTask();
}

boost::intrusive_ptr<impl::TaskContext> Queue::PopBlocking() {
    Worker* const worker = GetLocalWorker();
    UASSERT(worker != nullptr);

    impl::TaskContext* const context = worker->PopBlocking();

    return boost::intrusive_ptr<impl::TaskContext>{context, /* add_ref= */ false};
}

void Queue::StopProcessing() noexcept { coordinator_.RequestStop(); }

std::size_t Queue::GetSizeApproximate() const noexcept {
    std::size_t size = global_queue_.GetSize();
    for (const Worker& worker : workers_) {
        size += worker.GetLocalQueueSize();
    }
    return size;
}

}  // namespace engine::fast

USERVER_NAMESPACE_END
