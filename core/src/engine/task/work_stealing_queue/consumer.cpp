#include <engine/task/work_stealing_queue/consumer.hpp>

#include <cstddef>
#include <memory>
#include "userver/utils/assert.hpp"

#ifdef __linux__
#include <linux/futex.h>
#include <sys/syscall.h>
#endif

#include <userver/utils/rand.hpp>
#include <userver/utils/span.hpp>

#include <engine/task/task_context.hpp>
#include <engine/task/work_stealing_queue/global_queue.hpp>
#include <engine/task/work_stealing_queue/task_queue.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine {
#ifdef __linux__
namespace {
void FutexWait(std::atomic<std::int32_t>* value, int expected_value) {
  syscall(SYS_futex, value, FUTEX_WAIT, expected_value, nullptr, nullptr, 0);
}

void FutexWake(std::atomic<std::int32_t>* value, int count) {
  syscall(SYS_futex, value, FUTEX_WAKE, count, nullptr, nullptr, 0);
}
}  // namespace
#endif

namespace {
constexpr std::size_t kDefaultStealSpins = 10000;
constexpr std::size_t kDefaultStealSize = 5;
// frequency of visits to the global
// queue to guarantee progress
constexpr std::size_t kFrequencyGlobalQueuePop = 61;
// frequency of visits to the background
// queue in stealing process
constexpr std::size_t kFrequencyStealingBackgroundQueuePop = 10;
}  // namespace

Consumer::Consumer(WorkStealingTaskQueue& owner,
                   ConsumersManager& consumers_manager)
    : owner_(owner),
      consumers_manager_(consumers_manager),
      steal_attempts_count_(std::max<std::size_t>(
          3, kDefaultStealSpins / owner.consumers_count_)),
      rnd_(utils::Rand()),
      steps_count_(rnd_()),
      global_queue_token_(owner_.global_queue_.CreateConsumerToken()),
      background_queue_token_(owner.background_queue_.CreateConsumerToken()) {}

void Consumer::Push(impl::TaskContext* ctx) {
  if (ctx && ctx->IsBackground()) {
    owner_.background_queue_.Push(background_queue_token_, ctx);
    return;
  }
  const std::size_t surplus_queue_size = local_queue_surplus_.GetSize();
  if (surplus_queue_size) {
    if (!local_queue_surplus_.TryPush(ctx)) {
      EmptySurplusQueue(ctx);
    }
    return;
  }
  if (!local_queue_.TryPush(ctx)) {
    // local_queue_surplus_ must be empty
    bool ok = local_queue_surplus_.TryPush(ctx);
    UASSERT(ok);
  }
}

impl::TaskContext* Consumer::PopBlocking() { return DoPop(); }

std::size_t Consumer::GetLocalQueueSize() const noexcept {
  return local_queue_.GetSize() + local_queue_surplus_.GetSize();
}

WorkStealingTaskQueue* Consumer::GetOwner() const noexcept { return &owner_; }

void Consumer::SetIndex(std::size_t index) noexcept { inner_index_ = index; }

bool Consumer::IsStopped() const noexcept {
  return consumers_manager_.IsStopped();
}

void Consumer::EmptySurplusQueue(impl::TaskContext* extra) {
  // Pop all tasks from surplus queue
  std::size_t free_tasks_count = local_queue_surplus_.TryPopBulk(
      utils::span(steal_buffer_.data(), kConsumerStealBufferSize));
  steal_buffer_[free_tasks_count++] = extra;

  // First, we fill the local queue with the maximum number of tasks
  const size_t pushed_shift = local_queue_.PushBulk(
      utils::span(steal_buffer_.data(), free_tasks_count));

  // Second, we push the remaining tasks to the global queue
  if (pushed_shift < free_tasks_count) {
    owner_.global_queue_.PushBulk(
        global_queue_token_, utils::span(steal_buffer_.data() + pushed_shift,
                                         free_tasks_count - pushed_shift));
  }
}

impl::TaskContext* Consumer::StealFromAnotherConsumerOrGlobalQueue(
    const std::size_t attempts, std::size_t to_steal_count) {
  std::size_t stealed_size = 0;
  for (std::size_t i = 0;
       i < attempts && to_steal_count > 0 && stealed_size == 0; ++i) {
    std::size_t start_index = rnd_() % owner_.consumers_count_;
    for (std::size_t shift = 0;
         shift < owner_.consumers_count_ && to_steal_count > 0; ++shift) {
      std::size_t index = (start_index + shift) % owner_.consumers_count_;
      Consumer* victim = &owner_.consumers_[index];
      if (victim == this) {
        continue;
      }
      const std::size_t tasks_count = victim->Steal(
          utils::span(steal_buffer_.data() + stealed_size, to_steal_count));
      stealed_size += tasks_count;
      to_steal_count -= tasks_count;
    }

    if (to_steal_count > 0) {
      impl::TaskContext* ctx = owner_.global_queue_.TryPop(global_queue_token_);
      if (ctx) {
        steal_buffer_[stealed_size++] = ctx;
        to_steal_count--;
      }
    }

    if (to_steal_count > 0 && i % kFrequencyStealingBackgroundQueuePop == 0) {
      impl::TaskContext* ctx =
          owner_.background_queue_.TryPop(background_queue_token_);
      if (ctx) {
        steal_buffer_[stealed_size++] = ctx;
        to_steal_count--;
      }
    }
  }
  if (stealed_size > 0) {
    impl::TaskContext* context = steal_buffer_[0];
    if (stealed_size > 1) {
      local_queue_.PushBulk(
          utils::span(steal_buffer_.data() + 1, stealed_size - 1));
    }
    return context;
  }
  return nullptr;
}

std::size_t Consumer::Steal(utils::span<impl::TaskContext*> buffer) {
  const size_t stealed_count = local_queue_.TryPopBulk(buffer);
  if (stealed_count) {
    return stealed_count;
  }
  return local_queue_surplus_.TryPopBulk(buffer);
}

impl::TaskContext* Consumer::TryPopFromOwnerQueue(const bool is_global) {
  GlobalQueue* queue = &owner_.global_queue_;
  GlobalQueue::Token* token = &global_queue_token_;
  if (!is_global) {
    queue = &owner_.background_queue_;
    token = &background_queue_token_;
  }
  const std::size_t consumers_count = owner_.consumers_count_;
  std::size_t steal_size = std::min(
      (queue->GetSizeApproximateDelayed() + consumers_count) / consumers_count,
      kConsumerStealBufferSize);
  steal_size =
      queue->PopBulk(*token, utils::span(steal_buffer_.data(), steal_size));
  if (steal_size == 0) {
    return nullptr;
  }
  impl::TaskContext* context = steal_buffer_[0];
  if (steal_size == 1) {
    return context;
  }
  local_queue_.PushBulk(utils::span(steal_buffer_.data() + 1, steal_size - 1));

  return context;
}

impl::TaskContext* Consumer::ProbabilisticPopFromOwnerQueues() {
  impl::TaskContext* context = nullptr;
  if (steps_count_ % kFrequencyGlobalQueuePop == 0) {
    context = owner_.global_queue_.TryPop(global_queue_token_);
    if (context) {
      return context;
    }
  }

  return nullptr;
}

impl::TaskContext* Consumer::TryPop() {
  impl::TaskContext* context = TryPopLocal();
  if (context) {
    return context;
  }

  context = TryPopFromOwnerQueue(/* is_global */ true);
  if (context) {
    return context;
  }

  if (consumers_manager_.AllowStealing()) {
    context = StealFromAnotherConsumerOrGlobalQueue(steal_attempts_count_,
                                                    kDefaultStealSize);
    bool last = consumers_manager_.StopStealing();

    // there are potentially other tasks that require a consumer
    if (last && context) {
      consumers_manager_.WakeUpOne();
    }
    if (context) {
      return context;
    }
  }

  context = TryPopFromOwnerQueue(/* is_global */ false);
  if (context) {
    return context;
  }

  return nullptr;
}

impl::TaskContext* Consumer::TryPopBeforeSleep() {
  impl::TaskContext* context = TryPopLocal();

  if (context) {
    return context;
  }

  context = StealFromAnotherConsumerOrGlobalQueue(1, 1);
  if (context) {
    return context;
  }

  context = owner_.background_queue_.TryPop(background_queue_token_);

  if (context) {
    return context;
  }
  return nullptr;
}

impl::TaskContext* Consumer::TryPopLocal() {
  impl::TaskContext* context = local_queue_.TryPop();
  if (context) {
    return context;
  }
  const size_t surplus_tasks_count = local_queue_surplus_.TryPopBulk(
      utils::span(steal_buffer_.data(), kConsumerStealBufferSize));
  if (surplus_tasks_count) {
    context = steal_buffer_[0];
    if (surplus_tasks_count > 1) {
      // In this case, the local queue is empty and `kConsumerStealBufferSize`
      // is less than its capacity
      local_queue_.PushBulk(
          utils::span(steal_buffer_.data() + 1, surplus_tasks_count - 1));
    }
  }
  return context;
}

impl::TaskContext* Consumer::DoPop() {
  ++steps_count_;
  impl::TaskContext* context = ProbabilisticPopFromOwnerQueues();
  if (context) {
    return context;
  }

  while (!IsStopped()) {
    context = TryPop();
    if (context) {
      return context;
    }
    const std::int32_t sleep_state = sleep_counter_.load();
    consumers_manager_.NotifySleep(this);

    context = TryPopBeforeSleep();

    if (context) {
      consumers_manager_.NotifyWakeUp(this);
      return context;
    }

    if (IsStopped()) {
      return nullptr;
    }

    Sleep(sleep_state);
    consumers_manager_.NotifyWakeUp(this);
  }
  return nullptr;
}

void Consumer::Sleep(const std::int32_t old_sleep_counter) {
#ifdef __linux__
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

void Consumer::WakeUp() {
#ifdef __linux__
  sleep_counter_.fetch_add(1);
  FutexWake(&sleep_counter_, 1);
#else
  std::unique_lock lk(mutex_);
  sleep_counter_.fetch_add(1);
  cv_.notify_one();
#endif
}

}  // namespace engine

USERVER_NAMESPACE_END
