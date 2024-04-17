
#pragma once
#include <atomic>
#include <cstddef>
#include <deque>
#include <mutex>
#include <vector>
#include "consumers_state.hpp"

USERVER_NAMESPACE_BEGIN

namespace engine {

class Consumer;

class ConsumersManager final {
 public:
  explicit ConsumersManager(std::size_t consumers_count);  //

  void NotifyNewTask();

  void NotifyWakeUp(Consumer* consumer);

  void NotifySleep(Consumer* consumer);

  bool AllowStealing();

  bool StopStealing();

  void WakeUpOne();

  void Stop();

  bool Stopped();

  std::size_t ConsumersCount() const { return consumers_count_; }

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
