#include <engine/task/task_queue_tsan.hpp>

#include <engine/task/task_context.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine {

namespace {

constexpr std::size_t kSemaphoreInitialCount = 0;

// Safe to use without compiler::ThreadLocal because no thread migrations possible with TaskQueueTSan.
thread_local std::size_t tsan_queue_thread_index = 0;

}  // namespace

TaskQueueTSan::TaskQueueTSan(const TaskProcessorConfig& config)
    : thread_pined_{utils::GenerateFixedArray(
          config.worker_threads,
          [&config](std::size_t /*index*/) {
              return ThreadTSanItem{moodycamel::LightweightSemaphore{kSemaphoreInitialCount, config.spinning_iterations}
              };
          }
      )}
{}

void TaskQueueTSan::Push(boost::intrusive_ptr<impl::TaskContext>&& context) {
    UASSERT(context);
    DoPush(context.get());
    context.detach();
}

boost::intrusive_ptr<impl::TaskContext> TaskQueueTSan::PopBlocking() {
    boost::intrusive_ptr<impl::TaskContext> context{DoPopBlocking(), /* add_ref= */ false};

    if (!context) {
        // return "stop" token back
        DoPushIntoThread(nullptr, tsan_queue_thread_index);
    }

    return context;
}

void TaskQueueTSan::StopProcessing() {
    const auto threads_count = thread_pined_.size();
    for (std::size_t i = 0; i < threads_count; ++i) {
        DoPushIntoThread(nullptr, i);
    }
}

std::size_t TaskQueueTSan::GetSizeApproximate() const noexcept {
    std::size_t sum = 0;
    for (const auto& item : thread_pined_) {
        sum += item.queue.size_approx();
    }
    return sum;
}

void TaskQueueTSan::PrepareWorker(std::size_t thread_index) { tsan_queue_thread_index = thread_index; }

std::size_t TaskQueueTSan::GetBestThreadForNewTask() const noexcept {
    return (round_robin_index_++ % thread_pined_.size());
}

void TaskQueueTSan::DoPush(impl::TaskContext* context) {
    UASSERT(context);

    auto thread_index = context->GetThreadPinning();
    if (thread_index == impl::TaskContext::kUnsetThreadIndex) {
        thread_index = GetBestThreadForNewTask();
        context->SetThreadPinning(thread_index);
    }
    DoPushIntoThread(context, thread_index);
}

void TaskQueueTSan::DoPushIntoThread(impl::TaskContext* context, std::size_t thread_index) {
    auto& item = thread_pined_[thread_index];

    // This piece of code is copy-pasted from
    // moodycamel::BlockingConcurrentQueue::enqueue
    item.queue.enqueue(context);
    item.semaphore.signal();
}

impl::TaskContext* TaskQueueTSan::DoPopBlocking() {
    impl::TaskContext* context{};

    auto& item = thread_pined_[tsan_queue_thread_index];

    // This piece of code is copy-pasted from moodycamel::BlockingConcurrentQueue::wait_dequeue
    item.semaphore.wait();
    while (!item.queue.try_dequeue(item.token, context)) {
        // Can happen when another consumer steals our item in exchange for another
        // item in a Moodycamel sub-queue that we have already passed.
    }

    return context;
}

}  // namespace engine

USERVER_NAMESPACE_END
