#pragma once
#include <atomic>
#include <cstdint>

class ConsumersState {
 public:
  struct State {
    uint32_t sleeping_count;
    uint32_t stealing_count;
  };

  ConsumersState() = default;

  ConsumersState(const ConsumersState& other);

  ConsumersState& operator=(const ConsumersState& other);

  bool TryIncrementStealersCount(const ConsumersState& old);

  State DerementStealersCount();

  State Get();

  void IncrementSleepingCount();

  void DecrementSleepingCount();

 private:
  State CreateState(uint64_t data);
  std::atomic<uint64_t> state_{0};
};
