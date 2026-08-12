#include <engine/task/task_queue_pull_pin.hpp>

#include <userver/utils/fast_scope_guard.hpp>

#include <engine/task/task_context.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine {

namespace {

constexpr auto kNotSet = std::numeric_limits<std::size_t>::max();
constexpr std::size_t kGlobalQueueMaxTimesIgnored = 4;

// Safe to use without compiler::ThreadLocal because no thread migrations possible with TaskQueuePullPin.
thread_local std::size_t pull_pin_queue_thread_index = kNotSet;

template <class T>
std::size_t PushAndReturnSize(T& item, impl::TaskContext* context) {
    const auto ok = item.queue.enqueue(context);
    UASSERT(ok);
    return ++item.size;
}

}  // namespace

TaskQueuePullPin::TaskQueuePullPin(const TaskProcessorConfig& config)
    : thread_pined_queue_{config.worker_threads, global_queue_}
{}

void TaskQueuePullPin::Push(boost::intrusive_ptr<impl::TaskContext>&& context) {
    UASSERT(context);
    DoPush(context.get());
    context.detach();
}

boost::intrusive_ptr<impl::TaskContext> TaskQueuePullPin::PopBlocking() {
    boost::intrusive_ptr<impl::TaskContext> context{DoPopBlocking(), /* add_ref= */ false};
    return context;
}

void TaskQueuePullPin::StopProcessing() {
    const auto threads_count = thread_pined_queue_.size();
    for (std::size_t i = 0; i < threads_count; ++i) {
        PushAndReturnSize(global_queue_, nullptr);
    }

    wait_wake_.WakeupAll();
}

std::size_t TaskQueuePullPin::GetSizeApproximate() const noexcept {
    std::size_t sum = 0;

    for (const auto& item : thread_pined_queue_) {
        sum += item.GetSizeApproximate();
    }
    sum += global_queue_.size;

    return sum;
}

void TaskQueuePullPin::PrepareWorker(std::size_t thread_index) { pull_pin_queue_thread_index = thread_index; }

void TaskQueuePullPin::DoPush(impl::TaskContext* context) {
    UASSERT(context);

    const auto threads_count = thread_pined_queue_.size();
    const auto thread_index = context->GetThreadPinning();

    // We can not trust `pull_pin_queue_thread_index` here, because current thread can belong to other
    // TaskQueuePullPin instance!

    if (thread_index == impl::TaskContext::kUnsetThreadIndex) {
        const auto size_snapshot = PushAndReturnSize(global_queue_, context);
        if (size_snapshot <= threads_count) {
            wait_wake_.WakeupSome(1);
        }
        return;
    }

    auto& local = thread_pined_queue_[thread_index];
    const auto size_snapshot = local.PushAndReturnSize(context);
    if (size_snapshot == 1) {
        const auto woken_up = wait_wake_.WakeupByIndex(thread_index);
        if (woken_up == 0) {
            // The thread with `thread_index` has not entered OS sleep yet or a race with `wait_wake_.WakeupSome`:
            //
            // `wait_wake_.WakeupSome` woken up the thread with `thread_index` to deal with global queue but we have
            // now a task in its local queue. Waking up one more thread, to catch up the global queue.
            if (global_queue_.size < threads_count) {
                wait_wake_.WakeupSome(1);
            }
        }
    }
}

impl::TaskContext* TaskQueuePullPin::DoPopBlocking() {
    impl::TaskContext* context{};

    const auto thread_index = pull_pin_queue_thread_index;
    auto& local = thread_pined_queue_[thread_index];

    local.StartNewContextRetrieval();
    wait_wake_.WaitByIndex(thread_index, [&]() { return local.TryExtract(global_queue_, context); });

    if (context && context->GetThreadPinning() == impl::TaskContext::kUnsetThreadIndex) {
        context->SetThreadPinning(thread_index);
    }

    return context;
}

std::size_t TaskQueuePullPin::ThreadPinnedQueue::PushAndReturnSize(ContextPtr context) {
    const auto ok = queue_.enqueue(context);
    UASSERT(ok);
    return ++size_;
}

bool TaskQueuePullPin::ThreadPinnedQueue::TryExtract(GlobalQueue& global, ContextPtr& context_out) {
    const utils::FastScopeGuard guard{[this]() noexcept { DecrementSizeIfWasNot(); }};

    // Make sure that Yields do not consume all the capacity and take task from global queue
    if (local_queue_fetch_counter_ >= kGlobalQueueMaxTimesIgnored && TryExtractFromGlobalQueue(global, context_out)) {
        return true;
    }

    if (TryExtractFromLocal(context_out)) {
        return true;
    }

    return TryExtractFromGlobalQueue(global, context_out);
}

void TaskQueuePullPin::ThreadPinnedQueue::StartNewContextRetrieval() noexcept {
    UASSERT(size_was_decremented_);
    size_was_decremented_ = false;
}

std::size_t TaskQueuePullPin::ThreadPinnedQueue::GetSizeApproximate() const noexcept { return size_.load(); }

void TaskQueuePullPin::ThreadPinnedQueue::DecrementSizeIfWasNot() noexcept {
    if (!std::exchange(size_was_decremented_, true)) {
        const auto items_count = --size_;
        UASSERT(items_count >= unsync_queue_.size());
    }
}

void TaskQueuePullPin::ThreadPinnedQueue::IncrementSizeIfNeeded() noexcept {
    if (size_was_decremented_) {
        ++size_;
    } else {
        size_was_decremented_ = true;
    }
}

bool TaskQueuePullPin::ThreadPinnedQueue::TryExtractFromLocal(ContextPtr& out_context) {
    if (TryExtractFromUnsyncQueue(out_context)) {
        return true;
    }

    MoveToUnsyncQueue();
    return TryExtractFromUnsyncQueue(out_context);
}

bool TaskQueuePullPin::ThreadPinnedQueue::TryExtractFromUnsyncQueue(ContextPtr& out_context) {
    if (unsync_queue_.empty()) {
        return false;
    }

    out_context = unsync_queue_.front();
    unsync_queue_.erase(unsync_queue_.begin());
    return true;
}

std::size_t TaskQueuePullPin::ThreadPinnedQueue::MoveToUnsyncQueue() {
    UASSERT(unsync_queue_.capacity());
    UASSERT(unsync_queue_.empty());

    // We must decrement `size` before retrieving queue data to ensure the TaskQueuePullPin::DoPush can properly
    // determinate that the WakeupByIndex must be called.
    DecrementSizeIfWasNot();

    unsync_queue_.resize(unsync_queue_.capacity(), boost::container::default_init);
    const auto dequeued_count = queue_.try_dequeue_bulk(consumer_token_, unsync_queue_.data(), unsync_queue_.size());
    unsync_queue_.resize(dequeued_count, boost::container::default_init);

    ++local_queue_fetch_counter_;
    return dequeued_count;
}

bool TaskQueuePullPin::ThreadPinnedQueue::TryExtractFromGlobalQueue(GlobalQueue& global, ContextPtr& out_context) {
    local_queue_fetch_counter_ = 0;

    while (global.size) {
        if (!global.queue.try_dequeue(global_queue_consumer_token_, out_context)) {
            continue;
        }

        if (!out_context) {
            global.queue.enqueue(nullptr);
            TryExtractFromLocal(out_context);  // Shutting down in progress, extracting local tasks first
            return true;                       // out_context is either a local task or nullptr
        }

        IncrementSizeIfNeeded();
        --global.size;
        return true;
    }

    return false;
}

}  // namespace engine

USERVER_NAMESPACE_END
