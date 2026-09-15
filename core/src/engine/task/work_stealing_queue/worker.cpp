#include <engine/task/work_stealing_queue/worker.hpp>

#ifdef __linux__

#include <linux/futex.h>
#include <sys/syscall.h>

#endif

#include <algorithm>

#include <userver/utils/assert.hpp>
#include <userver/utils/rand.hpp>

#include <engine/task/task_context.hpp>
#include <engine/task/work_stealing_queue/queue.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::fast {

namespace {

constexpr std::size_t kFrequencyGlobalQueuePop = 61;
constexpr std::size_t kDefaultStealSize = 3;
constexpr std::size_t kStealSeries = 2;
constexpr std::size_t kGrabBatchCap = 48;

#ifdef __linux__
void FutexWait(std::atomic<std::int32_t>* value, int expected_value) {
    syscall(SYS_futex, value, FUTEX_WAIT, expected_value, nullptr, nullptr, 0);
}

void FutexWake(std::atomic<std::int32_t>* value, int count) {
    syscall(SYS_futex, value, FUTEX_WAKE, count, nullptr, nullptr, 0);
}
#endif

}  // namespace

Worker::Worker(
    Queue& owner,
    Coordinator& coordinator,
    const std::size_t index,
    const std::size_t workers_count
) noexcept
    : owner_(owner),
      coordinator_(coordinator),
      index_(index),
      workers_count_(workers_count),
      rnd_(utils::Rand())
{}

void Worker::Push(impl::TaskContext* context) {
    UASSERT(context);

    if (!local_queue_.TryPush(context)) {
        OffloadToGlobal(context);
    }
}

impl::TaskContext* Worker::TryPopFromGlobalQueue() noexcept {
    boost::intrusive_ptr<impl::TaskContext> next = owner_.global_queue_.TryPop();
    if (!next) {
        return nullptr;
    }
    return next.detach();
}

impl::TaskContext* Worker::GrabTasksFromGlobalQueue() noexcept {
    const std::size_t
        grab_count = owner_.global_queue_.Grab(std::span{steal_buffer_.data(), kGrabBatchCap}, workers_count_);
    if (grab_count == 0) {
        return nullptr;
    }

    impl::TaskContext* const next = steal_buffer_[0];
    if (grab_count > 1) {
        local_queue_.PushMany(std::span{steal_buffer_.data() + 1, grab_count - 1});
    }
    return next;
}

std::size_t Worker::StealTasks(std::span<impl::TaskContext*> out_buffer) noexcept {
    return local_queue_.Grab(out_buffer);
}

impl::TaskContext* Worker::TryStealTasks(std::size_t series) noexcept {
    std::size_t to_steal = kDefaultStealSize;
    std::size_t stolen = 0;

    for (std::size_t t = 0; t < series && to_steal > 0 && stolen == 0; ++t) {
        const std::size_t k = rnd_();
        for (std::size_t i = 0; i < workers_count_ && to_steal > 0 && stolen == 0; ++i) {
            const std::size_t victim_index = (k + i) % workers_count_;
            if (victim_index == index_) {
                continue;
            }

            Worker& victim = owner_.workers_[victim_index];
            const std::size_t stolen_from_victim = victim.StealTasks(std::span{steal_buffer_.data() + stolen, to_steal}
            );
            if (stolen_from_victim > 0) {
                UASSERT(to_steal >= stolen_from_victim);
                to_steal -= stolen_from_victim;
                stolen += stolen_from_victim;
            }
        }
    }

    if (stolen == 0) {
        return nullptr;
    }

    impl::TaskContext* const next = steal_buffer_[0];
    if (stolen > 1) {
        local_queue_.PushMany(std::span{steal_buffer_.data() + 1, stolen - 1});
    }
    return next;
}

impl::TaskContext* Worker::TryPop() noexcept {
    if (iter_ % kFrequencyGlobalQueuePop == 0) {
        if (impl::TaskContext* next = TryPopFromGlobalQueue(); next != nullptr) {
            return next;
        }
    }

    if (impl::TaskContext* next = local_queue_.TryPop(); next != nullptr) {
        return next;
    }

    if (impl::TaskContext* next = GrabTasksFromGlobalQueue(); next != nullptr) {
        return next;
    }

    if (coordinator_.StartSpinning()) {
        impl::TaskContext* next = TryStealTasks(kStealSeries);
        const bool last_spinner = coordinator_.StopSpinning();

        if (next != nullptr && last_spinner) {
            coordinator_.WakeWorker();
        }

        if (next != nullptr) {
            return next;
        }
    }

    return nullptr;
}

impl::TaskContext* Worker::TryPopBeforeSleep() noexcept {
    if (impl::TaskContext* next = TryPopFromGlobalQueue(); next != nullptr) {
        return next;
    }

    return TryStealTasks(1);
}

void Worker::OffloadToGlobal(impl::TaskContext* overflow) noexcept {
    const std::size_t to_offload = local_queue_.GetSizeUpperBound() / 2 + 1;
    const std::size_t batch_size = local_queue_.Grab(std::span{steal_buffer_.data(), to_offload});
    UASSERT(batch_size < steal_buffer_.size());
    steal_buffer_[batch_size] = overflow;
    owner_.global_queue_.Offload(std::span{steal_buffer_.data(), batch_size + 1});
}

impl::TaskContext* Worker::PopBlocking() {
    ++iter_;

    while (!coordinator_.IsStopRequested()) {
        if (impl::TaskContext* next = TryPop(); next != nullptr) {
            return next;
        }

        const std::int32_t sleep_state = sleep_counter_.load();
        coordinator_.StepDownAsActiveWorker(*this);

        if (impl::TaskContext* next = TryPopBeforeSleep(); next != nullptr) {
            coordinator_.BecomeActiveWorker(*this);
            return next;
        }

        if (coordinator_.IsStopRequested()) {
            coordinator_.BecomeActiveWorker(*this);
            return nullptr;
        }

        Sleep(sleep_state);
        coordinator_.BecomeActiveWorker(*this);
    }

    coordinator_.BecomeActiveWorker(*this);
    return nullptr;
}

void Worker::Wake() noexcept {
#ifdef __linux__
    sleep_counter_.fetch_add(1);
    FutexWake(&sleep_counter_, 1);
#else
    const std::lock_guard lk(mutex_);
    sleep_counter_.fetch_add(1);
    cv_.notify_one();
#endif
}

void Worker::Sleep(const std::int32_t old_sleep_counter) noexcept {
#ifdef __linux__
    // std::atomic<>::wait is slow => use futex.
    while (old_sleep_counter == sleep_counter_.load()) {
        FutexWait(&sleep_counter_, old_sleep_counter);
    }
#else
    std::unique_lock lk(mutex_);
    while (old_sleep_counter == sleep_counter_.load()) {
        cv_.wait(lk);
    }
#endif
}

Queue* Worker::GetOwner() const noexcept { return &owner_; }

std::size_t Worker::GetLocalQueueSize() const noexcept { return local_queue_.GetSizeUpperBound(); }

}  // namespace engine::fast

USERVER_NAMESPACE_END
