#include "consumers_state.hpp"
#include <userver/utils/assert.hpp>

constexpr uint64_t kConsumersSleepingShift = static_cast<uint64_t>(1) << 32;

ConsumersState::ConsumersState(const ConsumersState& other)
    : state_(other.state_.load()) {}

ConsumersState& ConsumersState::operator=(const ConsumersState& other) {
  if (this == &other) {
    return *this;
  }
  state_.store(other.state_.load());
  return *this;
}

bool ConsumersState::TryIncrementStealersCount(const ConsumersState& old) {
  uint64_t old_state = old.state_.load();
  return state_.compare_exchange_strong(old_state, old_state + 1);
}

ConsumersState::State ConsumersState::Get() {
  return CreateState(state_.load());
}

ConsumersState::State ConsumersState::DerementStealersCount() {
  uint64_t old = state_.fetch_sub(1);
  return CreateState(old);
}

void ConsumersState::IncrementSleepingCount() {
  state_.fetch_add(kConsumersSleepingShift);
}

void ConsumersState::DecrementSleepingCount() {
  state_.fetch_sub(kConsumersSleepingShift);
}

ConsumersState::State ConsumersState::CreateState(uint64_t data) {
  return ConsumersState::State{
      static_cast<uint32_t>(data >> 32),
      static_cast<uint32_t>(data & (kConsumersSleepingShift - 1))};
}
