#include "consumer.hpp"
#include <linux/futex.h>
#include <sys/syscall.h>
#include "task_queue.hpp"

void FutexWait(std::atomic<int>* value, int expected_value) {
  syscall(SYS_futex, value, FUTEX_WAIT, expected_value, nullptr, nullptr, 0);
}

void FutexWake(std::atomic<int>* value, int count) {
  syscall(SYS_futex, value, FUTEX_WAKE, count, nullptr, nullptr, 0);
}

USERVER_NAMESPACE_BEGIN

namespace engine {

Consumer::Consumer()
    : owner_(nullptr),
      consumers_manager_(nullptr),
      inner_index_(0),
      default_steal_size_(2),
      steal_buffer_(),
      rnd_(reinterpret_cast<std::uintptr_t>(this)),
      steps_count_(rnd_()),
      pushed_(rnd_()),
      sleep_counter_(0) {}

Consumer::Consumer(WorkStealingTaskQueue* owner,
                   ConsumersManager* consumers_manager, std::size_t inner_index)
    : owner_(owner),
      consumers_manager_(consumers_manager),
      inner_index_(inner_index),
      default_steal_size_(2),
      steal_buffer_(),
      rnd_(reinterpret_cast<std::uintptr_t>(this)),
      steps_count_(rnd_()),
      pushed_(rnd_()),
      sleep_counter_(0) {}

Consumer::Consumer(const Consumer& rhs)
    : owner_(rhs.owner_),
      consumers_manager_(rhs.consumers_manager_),
      inner_index_(rhs.inner_index_),
      default_steal_size_(rhs.default_steal_size_),
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

std::size_t Consumer::LocalQueueSize() const { return local_queue_.Size(); }

WorkStealingTaskQueue* Consumer::GetOwner() const { return owner_; }

bool Consumer::Stopped() { return consumers_manager_->Stopped(); }

void Consumer::MoveTasksToGlobalQueue(impl::TaskContext* extra) {
  std::size_t size = local_queue_.TryPopBulk(steal_buffer_.data(),
                                             kConsumerStealBufferSize - 1);
  steal_buffer_[size] = extra;
  size++;
  owner_->global_queue_.PushBulk(steal_buffer_.data(), size);
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
      std::size_t tasks_count =
          victim->Steal(steal_buffer_.data() + stealed_size, to_steal_count);
      stealed_size += tasks_count;
      to_steal_count -= tasks_count;
    }
  }
  if (stealed_size > 0) {
    impl::TaskContext* context = steal_buffer_[0];
    if (stealed_size > 1) {
      local_queue_.PushBulk(steal_buffer_.data() + 1, stealed_size - 1);
    }
    return context;
  }
  return nullptr;
}

std::size_t Consumer::Steal(impl::TaskContext** buffer, std::size_t size) {
  return local_queue_.TryPopBulk(buffer, size);
}

impl::TaskContext* Consumer::PopFromGlobalQueue() {
  std::size_t consumers_count = owner_->consumers_count_;
  std::size_t steal_size = std::min(
      (owner_->global_queue_.Size() + consumers_count - 1) / consumers_count,
      kConsumerStealBufferSize);
  steal_size = owner_->global_queue_.PopBulk(steal_buffer_.data(), steal_size);
  if (steal_size == 0) {
    return nullptr;
  }
  impl::TaskContext* context = steal_buffer_[0];
  if (steal_size == 1) {
    return context;
  }
  local_queue_.PushBulk(steal_buffer_.data() + 1, steal_size - 1);

  return context;
}

impl::TaskContext* Consumer::TryPop() {
  ++steps_count_;
  impl::TaskContext* context = nullptr;

  if (steps_count_ % 61 == 0) {
    context = owner_->global_queue_.Pop();
    if (context) {
      return context;
    }
  }

  context = local_queue_.Pop();
  if (context) {
    return context;
  }

  context = PopFromGlobalQueue();
  if (context) {
    return context;
  }

  if (consumers_manager_->AllowStealing()) {
    context = StealFromAnotherConsumer(3, default_steal_size_);
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
  while (!Stopped()) {
    context = TryPop();
    if (context) {
      return context;
    }
    int sleep_state = sleep_counter_.load();
    consumers_manager_->NotifySleep(this);

    context = owner_->global_queue_.Pop();

    if (context) {
      consumers_manager_->NotifyWakeUp(this);
      return context;
    }

    context = StealFromAnotherConsumer(1, 1);

    if (context) {
      consumers_manager_->NotifyWakeUp(this);
      return context;
    }

    if (consumers_manager_->Stopped()) {
      return nullptr;
    }

    Sleep(sleep_state);
    consumers_manager_->NotifyWakeUp(this);
  }
  return nullptr;
}

void Consumer::Sleep(int val) {
  while (val == sleep_counter_.load()) {
    FutexWait(&sleep_counter_, val);
  }
}

void Consumer::WakeUp() {
  sleep_counter_.fetch_add(1);
  FutexWake(&sleep_counter_, 1);
}

}  // namespace engine

USERVER_NAMESPACE_END
