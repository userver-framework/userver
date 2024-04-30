#include <engine/task/work_stealing_queue/consumers_manager.hpp>

#include <mutex>

#include <engine/task/work_stealing_queue/consumer.hpp>
#include <engine/task/work_stealing_queue/consumers_state.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine {

ConsumersManager::ConsumersManager(std::size_t consumers_count)
    : consumers_count_(consumers_count), is_sleeping_(consumers_count, false) {}

void ConsumersManager::NotifyNewTask() {
  ConsumersState::State curr_state = state_.Get();
  UASSERT(curr_state.sleeping_count <= consumers_count_);
  UASSERT(curr_state.stealing_count <= consumers_count_);
  UASSERT(curr_state.stealing_count + curr_state.sleeping_count <=
          consumers_count_);
  if (curr_state.sleeping_count > 0 && curr_state.stealing_count < 1) {
    WakeUpOne();
  }
}

void ConsumersManager::NotifyWakeUp(Consumer* const consumer) {
  std::lock_guard lock_(mutex_);
  if (!is_sleeping_[consumer->inner_index_]) {
    return;
  }
  is_sleeping_[consumer->inner_index_] = false;
  state_.DecrementSleepingCount();
}

void ConsumersManager::NotifySleep(Consumer* const consumer) {
  std::lock_guard lock_(mutex_);

  if (is_sleeping_[consumer->inner_index_]) {
    return;
  }
  is_sleeping_[consumer->inner_index_] = true;
  sleep_dq_.push_back(consumer);
  state_.IncrementSleepingCount();
}

bool ConsumersManager::AllowStealing() noexcept {
  while (true) {
    ConsumersState curr_consumers_state = state_;
    ConsumersState::State curr_state = curr_consumers_state.Get();
    if (curr_state.stealing_count * 2 >= consumers_count_) {
      return false;
    }
    if (state_.TryIncrementStealersCount(curr_consumers_state)) {
      return true;
    }
  }
}

bool ConsumersManager::StopStealing() noexcept {
  ConsumersState::State old_state = state_.DerementStealersCount();
  return old_state.stealing_count == 1;
}

void ConsumersManager::WakeUpOne() {
  Consumer* consumer = nullptr;
  {
    std::lock_guard lock_(mutex_);
    while (!sleep_dq_.empty()) {
      Consumer* candidate = sleep_dq_.front();
      sleep_dq_.pop_front();
      if (is_sleeping_[candidate->inner_index_]) {
        is_sleeping_[candidate->inner_index_] = false;
        consumer = candidate;
        break;
      }
    }
  }
  if (consumer) {
    state_.DecrementSleepingCount();
    consumer->WakeUp();
  }
}

void ConsumersManager::Stop() noexcept {
  stopped_.store(true);
  WakeUpAll();
}

bool ConsumersManager::IsStopped() const noexcept { return stopped_.load(); }

void ConsumersManager::WakeUpAll() {
  std::vector<Consumer*> consumers;
  consumers.reserve(consumers_count_);
  {
    std::lock_guard lock_(mutex_);
    while (!sleep_dq_.empty()) {
      Consumer* consumer = sleep_dq_.front();
      sleep_dq_.pop_front();
      if (is_sleeping_[consumer->inner_index_]) {
        is_sleeping_[consumer->inner_index_] = false;
        consumers.push_back(consumer);
      }
    }
  }
  for (auto consumer : consumers) {
    state_.DecrementSleepingCount();
    consumer->WakeUp();
  }
}

}  // namespace engine

USERVER_NAMESPACE_END
