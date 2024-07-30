#pragma once

#include <atomic>
#include <vector>

#include <moodycamel/concurrentqueue.h>

#include <engine/coro/pool_config.hpp>
#include <engine/coro/pool_stats.hpp>
#include <engine/coro/stack_usage_monitor.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::coro {

class Pool final {
 public:
  using Task = engine::impl::TaskContext;

  using Coroutine = typename boost::coroutines2::coroutine<Task*>::push_type;
  class CoroutinePtr;
  using TaskPipe = typename boost::coroutines2::coroutine<Task*>::pull_type;
  using Executor = void (*)(TaskPipe&);

  Pool(PoolConfig config, Executor executor);
  ~Pool();

  CoroutinePtr GetCoroutine();
  void PutCoroutine(CoroutinePtr&& coroutine_ptr);
  PoolStats GetStats() const;
  std::size_t GetStackSize() const;
  void PrepareLocalCache();
  void ClearLocalCache();

  void RegisterThread();
  void AccountStackUsage();

 private:
  static PoolConfig FixupConfig(PoolConfig&& config);

  Coroutine CreateCoroutine(bool quiet = false);
  void OnCoroutineDestruction() noexcept;

  bool TryPopulateLocalCache();
  void DepopulateLocalCache();

  template <typename Token>
  Token& GetUsedPoolToken();

  const PoolConfig config_;
  const Executor executor_;

  // Maximum number of coroutines exchanged between used_coroutines_ for thread
  // local coroutine cache.
  const std::size_t local_coroutine_move_size_;

  // Reduces contention by allowing bulk operations on used_coroutines_.
  // Coroutines in local_coro_buffer_ are counted as used in statistics.
  // Unprotected thread_local is OK here, because coro::Pool is always used
  // outside of any coroutine.
  static inline thread_local std::vector<Coroutine> local_coro_buffer_;

  boost::coroutines2::protected_fixedsize_stack stack_allocator_;
  // Some pointers arithmetic in StackUsageMonitor depends on this.
  // If you change the allocator, adjust the math there accordingly.
  static_assert(std::is_same_v<decltype(stack_allocator_),
                               boost::coroutines2::protected_fixedsize_stack>);
  StackUsageMonitor stack_usage_monitor_;

  // We aim to reuse coroutines as much as possible,
  // because since coroutine stack is a mmap-ed chunk of memory and not actually
  // an allocated memory we don't want to de-virtualize that memory excessively.
  //
  // The same could've been achieved with some LIFO container, but apparently
  // we don't have a container handy enough to not just use 2 queues.
  moodycamel::ConcurrentQueue<Coroutine> initial_coroutines_;
  moodycamel::ConcurrentQueue<Coroutine> used_coroutines_;

  std::atomic<std::size_t> idle_coroutines_num_;
  std::atomic<std::size_t> total_coroutines_num_;
};

class Pool::CoroutinePtr final {
 public:
  CoroutinePtr(Coroutine&& coro, Pool& pool) noexcept;

  CoroutinePtr(CoroutinePtr&&) noexcept = default;
  CoroutinePtr& operator=(CoroutinePtr&&) noexcept = default;

  ~CoroutinePtr();

  Coroutine& Get() noexcept;

  void ReturnToPool() &&;

 private:
  Coroutine coro_;
  Pool* pool_;
};

}  // namespace engine::coro

USERVER_NAMESPACE_END
