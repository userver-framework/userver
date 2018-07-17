#pragma once

#include <algorithm>
#include <atomic>
#include <functional>

#include <boost/coroutine/asymmetric_coroutine.hpp>
#include <boost/lockfree/stack.hpp>

#include <logging/log.hpp>

#include "pool_config.hpp"
#include "pool_stats.hpp"

namespace engine {
namespace coro {

template <typename Task>
class Pool {
 public:
  using Coroutine =
      typename boost::coroutines::asymmetric_coroutine<Task*>::push_type;
  using TaskPipe =
      typename boost::coroutines::asymmetric_coroutine<Task*>::pull_type;
  using Executor = std::function<void(TaskPipe&)>;

  Pool(PoolConfig config, Executor executor);
  ~Pool();

  Coroutine* GetCoroutine();
  void PutCoroutine(Coroutine* coroutine);
  PoolStats GetStats() const;

 private:
  Coroutine* CreateCoroutine(bool quiet = false);
  void DestroyCoroutine(Coroutine* coroutine) noexcept;

  const PoolConfig config_;
  const Executor executor_;

  boost::lockfree::stack<Coroutine*, boost::lockfree::fixed_sized<true>>
      coroutines_;
  std::atomic<size_t> active_coroutines_num_;
  std::atomic<size_t> total_coroutines_num_;
};

template <typename Task>
Pool<Task>::Pool(PoolConfig config, Executor executor)
    : config_(std::move(config)),
      executor_(std::move(executor)),
      coroutines_(config.max_size),
      active_coroutines_num_(0),
      total_coroutines_num_(0) {
  for (auto i = config_.initial_size; i > 0; --i) {
    coroutines_.push(CreateCoroutine(/*quiet =*/true));
  }
}

template <typename Task>
Pool<Task>::~Pool() {
  Coroutine* coroutine = nullptr;
  while (coroutines_.pop(coroutine)) {
    DestroyCoroutine(coroutine);
  }
}

template <typename Task>
typename Pool<Task>::Coroutine* Pool<Task>::GetCoroutine() {
  Coroutine* coroutine = nullptr;
  if (!coroutines_.pop(coroutine)) {
    coroutine = CreateCoroutine();
  }
  ++active_coroutines_num_;
  return coroutine;
}

template <typename Task>
void Pool<Task>::PutCoroutine(Coroutine* coroutine) {
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
typename Pool<Task>::Coroutine* Pool<Task>::CreateCoroutine(bool quiet) {
  auto new_total = total_coroutines_num_++;
  auto* coroutine = new Coroutine(executor_);
  if (!quiet) {
    LOG_DEBUG() << "Created a coroutine #" << new_total << '/'
                << config_.max_size;
  }
  return coroutine;
}

template <typename Task>
void Pool<Task>::DestroyCoroutine(Coroutine* coroutine) noexcept {
  delete coroutine;
  --total_coroutines_num_;
}

}  // namespace coro
}  // namespace engine
