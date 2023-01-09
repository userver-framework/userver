#include <engine/task/task_queue_pinned.hpp>

#include <cstdint>
#include <queue>

#include <concurrent/impl/intrusive_mpsc_queue.hpp>
#include <concurrent/impl/semaphore.hpp>
#include <engine/task/task_context.hpp>
#include <engine/task/task_processor_config.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/enumerate.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::impl {

namespace {

constexpr std::size_t kNonTaskProcessorThread = -1;
thread_local std::size_t task_processor_thread_index = kNonTaskProcessorThread;

struct HookExtractor final {
  auto& operator()(TaskContext& context) const noexcept {
    return context.GetTaskQueueHook();
  }
};

using MpscQueue =
    concurrent::impl::IntrusiveMpscQueue<TaskContext, HookExtractor>;

}  // namespace

struct alignas(64) TaskQueuePinned::SubQueue final {
  std::int64_t GetSizeApprox() const noexcept {
    return std::int64_t{incoming_count.GetCountApprox()};
  }

  MpscQueue incoming;
  concurrent::impl::Semaphore incoming_count{0};
  bool is_incoming_closed{false};
};

TaskQueuePinned::TaskQueuePinned(const TaskProcessorConfig& config)
    : sub_queues_(config.worker_threads) {
  UASSERT(config.worker_threads != 0);
}

TaskQueuePinned::~TaskQueuePinned() {
  UASSERT_MSG(GetSizeApprox() == 0,
              "Task queue must be depleted before destruction");
}

void TaskQueuePinned::Push(TaskContext& context) {
  UASSERT(!is_shut_down_.load(std::memory_order_relaxed));

  auto thread_index = context.GetThreadIndex();
  if (thread_index == TaskContext::kNotPinnedToThread) {
    thread_index = FindLeastOccupiedThread();
  }

  auto& sub_queue = sub_queues_[thread_index];
  sub_queue.incoming.Push(context);
  sub_queue.incoming_count.Post();
}

TaskContext* TaskQueuePinned::PopBlocking() {
  UASSERT(task_processor_thread_index != kNonTaskProcessorThread);
  auto& sub_queue = sub_queues_[task_processor_thread_index];

  if (sub_queue.is_incoming_closed) return nullptr;
  sub_queue.incoming_count.WaitBlocking();

  auto* const context = sub_queue.incoming.TryPop();
  if (!context) {
    UASSERT(is_shut_down_.load(std::memory_order_relaxed));
    sub_queue.is_incoming_closed = true;
  } else if (context->GetThreadIndex() == kNonTaskProcessorThread) {
    context->SetThreadIndex(task_processor_thread_index);
  } else {
    UASSERT(context->GetThreadIndex() == task_processor_thread_index);
  }
  return context;
}

void TaskQueuePinned::SetupThread() noexcept {
  const auto index =
      next_unowned_sub_queue_.fetch_add(1, std::memory_order_relaxed);
  UASSERT(index < sub_queues_.size());
  task_processor_thread_index = index;
}

void TaskQueuePinned::Shutdown() noexcept {
  // Shutdown is idempotent, no need to 'exchange'
  is_shut_down_.store(true, std::memory_order_relaxed);

  for (auto& sub_queue : sub_queues_) {
    // Signal without any item means sub-queue shutdown
    sub_queue.incoming_count.Post();
  }
}

std::size_t TaskQueuePinned::GetSizeApprox() const noexcept {
  std::int64_t total = 0;
  for (const auto& sub_queue : sub_queues_) {
    total += sub_queue.GetSizeApprox();
  }
  return std::max(total, std::int64_t{0});
}

std::size_t TaskQueuePinned::FindLeastOccupiedThread() const noexcept {
  std::int64_t min_size = std::numeric_limits<std::int64_t>::max();
  std::size_t best_thread_index = kNonTaskProcessorThread;

  // TODO remove
  thread_local std::vector<std::int64_t> sizes;
  sizes.reserve(sub_queues_.size());
  sizes.clear();

  for (const auto& [index, sub_queue] : utils::enumerate(sub_queues_)) {
    const auto size_approx = sub_queue.GetSizeApprox();
    sizes.push_back(size_approx);

    // In case of equally occupied threads, pick the first one. This allows some
    // threads with greater indices to fall asleep in case of low load.
    if (size_approx < min_size) {
      min_size = size_approx;
      best_thread_index = index;
    }
  }

  UASSERT(best_thread_index != kNonTaskProcessorThread);
  // TODO remove
  if constexpr (false) {
    fmt::print("Pinning a task to thread-id=#{}, queue-sizes=[{:2}]\n",
               best_thread_index, fmt::join(sizes, ","));
  }
  return best_thread_index;
}

}  // namespace engine::impl

USERVER_NAMESPACE_END
