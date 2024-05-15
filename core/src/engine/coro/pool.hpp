#pragma once

#include <algorithm>  // for std::max
#include <atomic>
#include <cerrno>
#include <iterator>
#include <optional>
#include <utility>

#include <moodycamel/concurrentqueue.h>

#include <coroutines/coroutine.hpp>

#include <userver/logging/log.hpp>
#include <userver/utils/assert.hpp>

#include "pool_config.hpp"
#include "pool_stats.hpp"

USERVER_NAMESPACE_BEGIN

namespace engine::coro {

template <typename Task>
class Pool final {
 public:
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
  void ClearLocalCache();

 private:
  Coroutine CreateCoroutine(bool quiet = false);
  bool TryPopulateLocalCache();
  void ReduceLocalCacheBufferSize(std::size_t new_size);
  void OnCoroutineDestruction() noexcept;

  template <typename Token>
  Token& GetUsedPoolToken();

  const PoolConfig config_;
  const Executor executor_;

  // Maximum thread local coroutine cache size
  static constexpr std::size_t kLocalCoroutineCacheMaxSize = 32;
  // Maximum number of coroutines exchanged between
  // used_coroutines_ for thread local coroutine cache
  // for sure kLocalCoroutineMoveSize <= kLocalCoroutineCacheMaxSize
  static constexpr std::size_t kLocalCoroutineMoveSize = 16;
  static_assert(kLocalCoroutineMoveSize <= kLocalCoroutineCacheMaxSize);

  static inline thread_local std::vector<Coroutine> local_coro_buffer_;

  boost::coroutines2::protected_fixedsize_stack stack_allocator_;

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

template <typename Task>
class Pool<Task>::CoroutinePtr final {
 public:
  CoroutinePtr(Coroutine&& coro, Pool<Task>& pool) noexcept
      : coro_(std::move(coro)), pool_(&pool) {}

  CoroutinePtr(CoroutinePtr&&) noexcept = default;
  CoroutinePtr& operator=(CoroutinePtr&&) noexcept = default;

  ~CoroutinePtr() {
    UASSERT(pool_);
    if (coro_) pool_->OnCoroutineDestruction();
  }

  Coroutine& Get() noexcept {
    UASSERT(coro_);
    return coro_;
  }

  void ReturnToPool() && {
    UASSERT(coro_);
    pool_->PutCoroutine(std::move(*this));
  }

 private:
  Coroutine coro_;
  Pool<Task>* pool_;
};

template <typename Task>
Pool<Task>::Pool(PoolConfig config, Executor executor)
    : config_(std::move(config)),
      executor_(executor),
      stack_allocator_(config_.stack_size),
      initial_coroutines_(config_.initial_size),
      used_coroutines_(config_.max_size),
      idle_coroutines_num_(config_.initial_size),
      total_coroutines_num_(0) {
  moodycamel::ProducerToken token(initial_coroutines_);
  for (std::size_t i = 0; i < config_.initial_size; ++i) {
    bool ok =
        initial_coroutines_.enqueue(token, CreateCoroutine(/*quiet =*/true));
    UINVARIANT(ok, "Failed to allocate the initial coro pool");
  }
}

template <typename Task>
Pool<Task>::~Pool() = default;

template <typename Task>
typename Pool<Task>::CoroutinePtr Pool<Task>::GetCoroutine() {
  struct CoroutineMover {
    std::optional<Coroutine>& result;

    CoroutineMover& operator=(Coroutine&& coro) {
      result.emplace(std::move(coro));
      return *this;
    }
  };
  std::optional<Coroutine> coroutine;
  CoroutineMover mover{coroutine};

  // First try to dequeue from 'working set': if we can get a coroutine
  // from there we are happy, because we saved on minor-page-faulting (thus
  // increasing resident memory usage) a not-yet-de-virtualized coroutine stack.
  if (!local_coro_buffer_.empty() || TryPopulateLocalCache()) {
    coroutine = std::move(local_coro_buffer_.back());
    local_coro_buffer_.pop_back();
  } else if (initial_coroutines_.try_dequeue(mover)) {
    idle_coroutines_num_.fetch_sub(1);
  } else {
    coroutine.emplace(CreateCoroutine());
  }

  return CoroutinePtr(std::move(*coroutine), *this);
}

template <typename Task>
void Pool<Task>::PutCoroutine(CoroutinePtr&& coroutine_ptr) {
  if (local_coro_buffer_.size() < kLocalCoroutineCacheMaxSize) {
    local_coro_buffer_.push_back(std::move(coroutine_ptr.Get()));
    return;
  }

  const std::size_t current_idle_coroutines_num = idle_coroutines_num_.load();
  if (current_idle_coroutines_num >= config_.max_size) {
    ReduceLocalCacheBufferSize(local_coro_buffer_.size() -
                               kLocalCoroutineMoveSize);
    return;
  }

  const std::size_t return_to_pool_from_local_cache_num = std::min(
      config_.max_size - current_idle_coroutines_num, kLocalCoroutineMoveSize);

  if (used_coroutines_.enqueue_bulk(
          GetUsedPoolToken<moodycamel::ProducerToken>(),
          std::make_move_iterator(local_coro_buffer_.begin()) +
              local_coro_buffer_.size() - return_to_pool_from_local_cache_num,
          return_to_pool_from_local_cache_num)) {
    idle_coroutines_num_.fetch_add(return_to_pool_from_local_cache_num);
  }

  ReduceLocalCacheBufferSize(local_coro_buffer_.size() -
                             kLocalCoroutineMoveSize);
  local_coro_buffer_.push_back(std::move(coroutine_ptr.Get()));
}

template <typename Task>
PoolStats Pool<Task>::GetStats() const {
  PoolStats stats;
  stats.active_coroutines =
      total_coroutines_num_.load() -
      (used_coroutines_.size_approx() + initial_coroutines_.size_approx());
  stats.total_coroutines =
      std::max(total_coroutines_num_.load(), stats.active_coroutines);
  return stats;
}

template <typename Task>
void Pool<Task>::ClearLocalCache() {
  const std::size_t current_idle_coroutines_num = idle_coroutines_num_.load();
  if (current_idle_coroutines_num < config_.max_size) {
    const std::size_t return_to_pool_from_local_cache_num =
        std::min(config_.max_size - current_idle_coroutines_num,
                 local_coro_buffer_.size());

    if (used_coroutines_.enqueue_bulk(
            GetUsedPoolToken<moodycamel::ProducerToken>(),
            std::make_move_iterator(local_coro_buffer_.begin()),
            return_to_pool_from_local_cache_num)) {
      idle_coroutines_num_.fetch_add(return_to_pool_from_local_cache_num);
    }
  }
  ReduceLocalCacheBufferSize(0);
}

template <typename Task>
typename Pool<Task>::Coroutine Pool<Task>::CreateCoroutine(bool quiet) {
  try {
    Coroutine coroutine(stack_allocator_, executor_);
    const auto new_total = ++total_coroutines_num_;
    if (!quiet) {
      LOG_DEBUG() << "Created a coroutine #" << new_total << '/'
                  << config_.max_size;
    }
    return coroutine;
  } catch (const std::bad_alloc&) {
    if (errno == ENOMEM) {
      // It should be ok to allocate here (which LOG_ERROR might do),
      // because ENOMEM is most likely coming from mmap
      // hitting vm.max_map_count limit, not from the actual memory limit.
      // See `stack_context::allocate` in
      // boost/context/posix/protected_fixedsize_stack.hpp
      LOG_ERROR() << "Failed to allocate a coroutine (ENOMEM), current "
                     "coroutines count: "
                  << total_coroutines_num_.load()
                  << "; are you hitting the vm.max_map_count limit?";
    }

    throw;
  }
}

template <typename Task>
bool Pool<Task>::TryPopulateLocalCache() {
  // using initial_coroutines_.size_approx() fast
  // because it has one producer queue
  const std::size_t deque_num =
      std::min(idle_coroutines_num_.load() - initial_coroutines_.size_approx(),
               kLocalCoroutineMoveSize);
  if (deque_num > 0) {
    const std::size_t dequed_num = used_coroutines_.try_dequeue_bulk(
        GetUsedPoolToken<moodycamel::ConsumerToken>(),
        std::back_inserter(local_coro_buffer_), deque_num);

    if (dequed_num > 0) {
      idle_coroutines_num_.fetch_sub(dequed_num);
      return true;
    }
  }
  return false;
}

template <typename Task>
void Pool<Task>::ReduceLocalCacheBufferSize(std::size_t new_size) {
  UASSERT(new_size <= local_coro_buffer_.size());
  while (local_coro_buffer_.size() > new_size) {
    local_coro_buffer_.pop_back();
  }
}

template <typename Task>
void Pool<Task>::OnCoroutineDestruction() noexcept {
  --total_coroutines_num_;
}

template <typename Task>
std::size_t Pool<Task>::GetStackSize() const {
  return config_.stack_size;
}

template <typename Task>
template <typename Token>
Token& Pool<Task>::GetUsedPoolToken() {
  thread_local Token token(used_coroutines_);
  return token;
}

}  // namespace engine::coro

USERVER_NAMESPACE_END
