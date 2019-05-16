#pragma once

#include <algorithm>
#include <atomic>
#include <functional>
#include <memory>

#include <boost/lockfree/stack.hpp>
#include <uboost_coro/coroutine2/coroutine.hpp>
#include <uboost_coro/coroutine2/protected_fixedsize_stack.hpp>

#include <logging/log.hpp>

#include "pool_config.hpp"
#include "pool_stats.hpp"

namespace engine {
namespace coro {

// FIXME: determine proper stack size (TAXICOMMON-138)
static constexpr size_t kStackSize = 256 * 1024ull;

template <typename Task>
class Pool {
 public:
  using Coroutine = typename boost::coroutines2::coroutine<Task*>::push_type;
  using CoroutinePtr =
      std::unique_ptr<Coroutine, std::function<void(Coroutine*)>>;
  using TaskPipe = typename boost::coroutines2::coroutine<Task*>::pull_type;
  using Executor = std::function<void(TaskPipe&)>;

  Pool(PoolConfig config, Executor executor);
  ~Pool();

  CoroutinePtr GetCoroutine();
  void PutCoroutine(CoroutinePtr&& coroutine_ptr);
  PoolStats GetStats() const;

 private:
  Coroutine* CreateCoroutine(bool quiet = false);
  void DestroyCoroutine(Coroutine* coroutine) noexcept;

  const PoolConfig config_;
  const Executor executor_;

  boost::coroutines2::protected_fixedsize_stack stack_allocator_;
  boost::lockfree::stack<Coroutine*, boost::lockfree::fixed_sized<true>>
      coroutines_;
  std::atomic<size_t> idle_coroutines_num_;
  std::atomic<size_t> total_coroutines_num_;
};

template <typename Task>
Pool<Task>::Pool(PoolConfig config, Executor executor)
    // NOLINTNEXTLINE(hicpp-move-const-arg,performance-move-const-arg)
    : config_(std::move(config)),
      executor_(std::move(executor)),
      stack_allocator_(kStackSize),
      // NOLINTNEXTLINE(clang-analyzer-core.uninitialized.UndefReturn)
      coroutines_(config.max_size),
      idle_coroutines_num_(0),
      total_coroutines_num_(0) {
  for (auto i = config_.initial_size; i > 0; --i) {
    coroutines_.push(CreateCoroutine(/*quiet =*/true));
  }
  idle_coroutines_num_ = config_.initial_size;
}

template <typename Task>
Pool<Task>::~Pool() {
  idle_coroutines_num_ = 0;
  Coroutine* coroutine = nullptr;
  while (coroutines_.pop(coroutine)) {
    DestroyCoroutine(coroutine);
  }
}

template <typename Task>
typename Pool<Task>::CoroutinePtr Pool<Task>::GetCoroutine() {
  Coroutine* coroutine = nullptr;
  // NOLINTNEXTLINE(clang-analyzer-core.uninitialized.UndefReturn)
  if (coroutines_.pop(coroutine)) {
    --idle_coroutines_num_;
  } else {
    coroutine = CreateCoroutine();
  }
  return CoroutinePtr(
      coroutine, [this](Coroutine* coroutine) { DestroyCoroutine(coroutine); });
}

template <typename Task>
void Pool<Task>::PutCoroutine(CoroutinePtr&& coroutine_ptr) {
  // NOLINTNEXTLINE(clang-analyzer-core.uninitialized.UndefReturn)
  if (coroutines_.bounded_push(coroutine_ptr.get())) {
    [[maybe_unused]] auto ptr = coroutine_ptr.release();
    ++idle_coroutines_num_;
  }
}

template <typename Task>
PoolStats Pool<Task>::GetStats() const {
  PoolStats stats;
  stats.active_coroutines =
      total_coroutines_num_.load() - idle_coroutines_num_.load();
  stats.total_coroutines =
      std::max(total_coroutines_num_.load(), stats.active_coroutines);
  return stats;
}

template <typename Task>
typename Pool<Task>::Coroutine* Pool<Task>::CreateCoroutine(bool quiet) {
  auto new_total = ++total_coroutines_num_;
  auto* coroutine = new Coroutine(stack_allocator_, executor_);
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
