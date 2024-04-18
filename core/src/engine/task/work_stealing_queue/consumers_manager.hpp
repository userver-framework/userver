
#pragma once
#include <atomic>
#include <cstddef>
#include <deque>
#include <engine/task/work_stealing_queue/consumers_state.hpp>
#include <mutex>
#include <vector>

USERVER_NAMESPACE_BEGIN

namespace engine {

class Consumer;

class ConsumersManager final {
 public:
  explicit ConsumersManager(std::size_t consumers_count);

  void NotifyNewTask();

  void NotifyWakeUp(Consumer* consumer);

  void NotifySleep(Consumer* consumer);

  bool AllowStealing() noexcept;

  bool StopStealing() noexcept;

  void WakeUpOne();

  void Stop() noexcept;

  bool IsStopped() const noexcept;

  std::size_t GetConsumersCount() const noexcept { return consumers_count_; }

 private:
  void WakeUpAll();
  const std::size_t consumers_count_;
  std::mutex mutex_;
  ConsumersState state_;
  std::atomic<bool> stopped_;
  std::deque<Consumer*> sleep_dq_;
  std::vector<bool> is_sleeping_;
};
}  // namespace engine

USERVER_NAMESPACE_END
