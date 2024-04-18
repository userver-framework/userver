#include <engine/task/work_stealing_queue/consumers_state.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine {

namespace {

constexpr std::uint64_t kConsumersSleepingShift = static_cast<std::uint64_t>(1)
                                                  << 32;

}

ConsumersState::ConsumersState(const ConsumersState& other)
    : state_(other.state_.load()) {}

ConsumersState& ConsumersState::operator=(const ConsumersState& other) {
  if (this == &other) {
    return *this;
  }
  state_.store(other.state_.load());
  return *this;
}

bool ConsumersState::TryIncrementStealersCount(
    const ConsumersState& old) noexcept {
  std::uint64_t old_state = old.state_.load();
  return state_.compare_exchange_strong(old_state, old_state + 1);
}

ConsumersState::State ConsumersState::Get() noexcept {
  return CreateState(state_.load());
}

ConsumersState::State ConsumersState::DerementStealersCount() noexcept {
  std::uint64_t old = state_.fetch_sub(1);
  return CreateState(old);
}

void ConsumersState::IncrementSleepingCount() noexcept {
  state_.fetch_add(kConsumersSleepingShift);
}

void ConsumersState::DecrementSleepingCount() noexcept {
  state_.fetch_sub(kConsumersSleepingShift);
}

ConsumersState::State ConsumersState::CreateState(std::uint64_t data) {
  return ConsumersState::State{
      static_cast<uint32_t>(data >> 32),
      static_cast<uint32_t>(data & (kConsumersSleepingShift - 1))};
}

}  // namespace engine

USERVER_NAMESPACE_END
