#pragma once

#include <algorithm>
#include <atomic>
#include <functional>

#include <boost/coroutine/symmetric_coroutine.hpp>
#include <boost/lockfree/stack.hpp>

#include <logging/log.hpp>

#include "pool_config.hpp"
#include "pool_stats.hpp"

namespace engine {
namespace coro {

template <typename Task>
class Pool {
 public:
  using CallType =
      typename boost::coroutines::symmetric_coroutine<Task*>::call_type;
  using YieldType =
      typename boost::coroutines::symmetric_coroutine<Task*>::yield_type;
  using Executor = std::function<void(YieldType&)>;

  Pool(PoolConfig config, Executor executor);
  ~Pool();

  CallType* GetCoroutine();
  void PutCoroutine(CallType* coroutine);
  PoolStats GetStats() const;

 private:
  CallType* CreateCoroutine(bool quiet = false);
  void DestroyCoroutine(CallType* coroutine) noexcept;

  const PoolConfig config_;
  const Executor executor_;

  boost::lockfree::stack<CallType*, boost::lockfree::fixed_sized<true>>
      coroutines_;
  std::atomic<size_t> active_coroutines_num_;
  std::atomic<size_t> total_coroutines_num_;
};

template <typename Task>
Pool<Task>::Pool(PoolConfig config, Executor executor)
    : config_(std::move(config)),
      executor_(std::move(executor)),
      active_coroutines_num_(0),
      total_coroutines_num_(0) {
  for (auto i = config_.initial_size; i > 0; --i) {
    coroutines_.push(CreateCoroutine(/*quiet =*/true));
  }
}

template <typename Task>
Pool<Task>::~Pool() {
  CallType* coroutine = nullptr;
  while (coroutines_.pop(coroutine)) {
    DestroyCoroutine(coroutine);
  }
}

template <typename Task>
typename Pool<Task>::CallType* Pool<Task>::GetCoroutine() {
  CallType* coroutine = nullptr;
  if (!coroutines_.pop(coroutine)) {
    coroutine = CreateCoroutine();
  }
  ++active_coroutines_num_;
  return coroutine;
}

template <typename Task>
void Pool<Task>::PutCoroutine(CallType* coroutine) {
  --active_coroutines_num_;
  if (!coroutines_.bounded_push(coroutine)) {
    DestroyCoroutine(coroutine);
  }
}

template <typename Task>
PoolStats Pool<Task>::GetStats() const {
  PoolStats stats;
  stats.active_coroutines = active_coroutines_num_.load();
  stats.total_coroutines =
      std::max(total_coroutines_num_.load(), stats.active_coroutines);
  return stats;
}

template <typename Task>
typename Pool<Task>::CallType* Pool<Task>::CreateCoroutine(bool quiet) {
  auto new_total = total_coroutines_num_++;
  auto* coroutine = new CallType(executor_);
  if (!quiet) {
    LOG_DEBUG() << "Created a coroutine #" << new_total << '/'
                << config_.max_size;
  }
  return coroutine;
}

template <typename Task>
void Pool<Task>::DestroyCoroutine(CallType* coroutine) noexcept {
  delete coroutine;
  --total_coroutines_num_;
}

}  // namespace coro
}  // namespace engine
