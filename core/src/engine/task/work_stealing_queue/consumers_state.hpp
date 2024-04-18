#pragma once
#include <atomic>
#include <cstdint>

USERVER_NAMESPACE_BEGIN

namespace engine {

class ConsumersState {
 public:
  struct State {
    std::uint32_t sleeping_count;
    std::uint32_t stealing_count;
  };

  ConsumersState() = default;

  ConsumersState(const ConsumersState& other);

  ConsumersState& operator=(const ConsumersState& other);

  bool TryIncrementStealersCount(const ConsumersState& old) noexcept;

  State DerementStealersCount() noexcept;

  State Get() noexcept;

  void IncrementSleepingCount() noexcept;

  void DecrementSleepingCount() noexcept;

 private:
  State CreateState(std::uint64_t data);
  std::atomic<std::uint64_t> state_{0};
};

}  // namespace engine

USERVER_NAMESPACE_END
