#include <engine/task/work_stealing_queue/consumer.hpp>

#include <engine/task/work_stealing_queue/task_queue.hpp>
#include <userver/utils/span.hpp>
#include "engine/task/task_context.hpp"

#ifdef __linux__
#include <linux/futex.h>
#include <sys/syscall.h>
#endif

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
constexpr std::size_t kConsumerStealBufferSize = 32;
constexpr std::size_t kDefaultStealAttempts = 3;
constexpr std::size_t kDefaultStealSize = 2;
}  // namespace

Consumer::Consumer(WorkStealingTaskQueue* owner,
                   ConsumersManager* consumers_manager)
    : owner_(owner),
      consumers_manager_(consumers_manager),
      steal_buffer_(),
      rnd_(reinterpret_cast<std::uintptr_t>(this)),
      steps_count_(rnd_()),
      pushed_(rnd_()),
      sleep_counter_(0) {}

bool Consumer::Push(impl::TaskContext* ctx) {
  pushed_++;
  if (pushed_ % 73 == 0) {
    return false;
  }
  if (!local_queue_.TryPush(ctx)) {
    MoveTasksToGlobalQueue(ctx);
  }
  return true;
}

impl::TaskContext* Consumer::Pop() { return DoPop(); }

std::size_t Consumer::LocalQueueSize() const noexcept {
  return local_queue_.Size();
}

WorkStealingTaskQueue* Consumer::GetOwner() const noexcept { return owner_; }

void Consumer::SetIndex(std::size_t index) noexcept { inner_index_ = index; }

bool Consumer::IsStopped() const noexcept {
  return consumers_manager_->IsStopped();
}

void Consumer::MoveTasksToGlobalQueue(impl::TaskContext* extra) {
  std::size_t size = local_queue_.TryPopBulk(
      utils::span(steal_buffer_.data(),
                  steal_buffer_.data() + kConsumerStealBufferSize - 1));
  steal_buffer_[size] = extra;
  size++;
  owner_->global_queue_.PushBulk(utils::span(
      steal_buffer_.data(),
      steal_buffer_.data() + size));  //)steal_buffer_.data(), size);
}

impl::TaskContext* Consumer::StealFromAnotherConsumer(
    std::size_t attempts, std::size_t to_steal_count) {
  std::size_t stealed_size = 0;
  for (std::size_t i = 0; i < attempts && to_steal_count > 0; ++i) {
    std::size_t start_index = rnd_() % owner_->consumers_count_;
    for (std::size_t shift = 0;
         shift < owner_->consumers_count_ && to_steal_count > 0; ++shift) {
      std::size_t index = (start_index + shift) % owner_->consumers_count_;
      Consumer* victim = &owner_->consumers_[index];
      if (victim == this) {
        continue;
      }
      std::size_t tasks_count = victim->Steal(
          utils::span(steal_buffer_.data() + stealed_size,
                      steal_buffer_.data() + stealed_size + to_steal_count));
      stealed_size += tasks_count;
      to_steal_count -= tasks_count;
    }
  }
  if (stealed_size > 0) {
    impl::TaskContext* context = steal_buffer_[0];
    if (stealed_size > 1) {
      local_queue_.PushBulk(utils::span(steal_buffer_.data() + 1,
                                        steal_buffer_.data() + stealed_size));
    }
    return context;
  }
  return nullptr;
}

std::size_t Consumer::Steal(utils::span<impl::TaskContext*> buffer) {
  return local_queue_.TryPopBulk(buffer);
}

impl::TaskContext* Consumer::TryPopFromGlobalQueue() {
  std::size_t consumers_count = owner_->consumers_count_;
  std::size_t steal_size = std::min(
      (owner_->global_queue_.Size() + consumers_count - 1) / consumers_count,
      kConsumerStealBufferSize);
  steal_size = owner_->global_queue_.PopBulk(
      utils::span(steal_buffer_.data(), steal_buffer_.data() + steal_size));
  if (steal_size == 0) {
    return nullptr;
  }
  impl::TaskContext* context = steal_buffer_[0];
  if (steal_size == 1) {
    return context;
  }
  local_queue_.PushBulk(
      utils::span(steal_buffer_.data() + 1, steal_buffer_.data() + steal_size));

  return context;
}

impl::TaskContext* Consumer::TryPop() {
  ++steps_count_;
  impl::TaskContext* context = nullptr;

  if (steps_count_ % 61 == 0) {
    context = owner_->global_queue_.TryPop();
    if (context) {
      return context;
    }
  }

  context = local_queue_.TryPop();
  if (context) {
    return context;
  }

  context = TryPopFromGlobalQueue();
  if (context) {
    return context;
  }

  if (consumers_manager_->AllowStealing()) {
    context =
        StealFromAnotherConsumer(kDefaultStealAttempts, kDefaultStealSize);
    bool last = consumers_manager_->StopStealing();
    if (last &&
        context) {  // there are potentially other tasks that require a consumer
      consumers_manager_->WakeUpOne();
    }
    if (context) {
      return context;
    }
  }

  return nullptr;
}

impl::TaskContext* Consumer::DoPop() {
  impl::TaskContext* context = nullptr;
  while (!IsStopped()) {
    context = TryPop();
    if (context) {
      return context;
    }
    std::int32_t sleep_state = sleep_counter_.load();
    consumers_manager_->NotifySleep(this);

    context = owner_->global_queue_.TryPop();

    if (context) {
      consumers_manager_->NotifyWakeUp(this);
      return context;
    }

    context = StealFromAnotherConsumer(1, 1);

    if (context) {
      consumers_manager_->NotifyWakeUp(this);
      return context;
    }

    if (IsStopped()) {
      return nullptr;
    }

    Sleep(sleep_state);
    consumers_manager_->NotifyWakeUp(this);
  }
  return nullptr;
}

void Consumer::Sleep(std::int32_t val) {
#ifdef __linux__
  while (val == sleep_counter_.load()) {
    FutexWait(&sleep_counter_, val);
  }
#else
  std::unique_lock lk(mutex_);
  while (val == sleep_counter_.load()) {
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
