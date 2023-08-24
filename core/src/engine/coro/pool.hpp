#pragma once

#include <algorithm>  // for std::max
#include <atomic>
#include <cerrno>
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

 private:
  Coroutine CreateCoroutine(bool quiet = false);
  void OnCoroutineDestruction() noexcept;

  template <typename Token>
  Token& GetToken();

  const PoolConfig config_;
  const Executor executor_;

  boost::coroutines2::protected_fixedsize_stack stack_allocator_;
  moodycamel::ConcurrentQueue<Coroutine> coroutines_;
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
      stack_allocator_(config.stack_size),
      coroutines_(config_.max_size),
      idle_coroutines_num_(config_.initial_size),
      total_coroutines_num_(0) {
  moodycamel::ProducerToken token(coroutines_);
  for (std::size_t i = 0; i < config_.initial_size; ++i) {
    bool ok = coroutines_.enqueue(token, CreateCoroutine(/*quiet =*/true));
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
  auto& token = GetToken<moodycamel::ConsumerToken>();
  if (coroutines_.try_dequeue(token, mover)) {
    --idle_coroutines_num_;
  } else {
    coroutine.emplace(CreateCoroutine());
  }
  return CoroutinePtr(std::move(*coroutine), *this);
}

template <typename Task>
void Pool<Task>::PutCoroutine(CoroutinePtr&& coroutine_ptr) {
  if (idle_coroutines_num_.load() >= config_.max_size) return;
  auto& token = GetToken<moodycamel::ProducerToken>();
  const bool ok = coroutines_.enqueue(token, std::move(coroutine_ptr.Get()));
  if (ok) ++idle_coroutines_num_;
}

template <typename Task>
PoolStats Pool<Task>::GetStats() const {
  PoolStats stats;
  stats.active_coroutines =
      total_coroutines_num_.load() - coroutines_.size_approx();
  stats.total_coroutines =
      std::max(total_coroutines_num_.load(), stats.active_coroutines);
  return stats;
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
void Pool<Task>::OnCoroutineDestruction() noexcept {
  --total_coroutines_num_;
}

template <typename Task>
std::size_t Pool<Task>::GetStackSize() const {
  return config_.stack_size;
}

template <typename Task>
template <typename Token>
Token& Pool<Task>::GetToken() {
  thread_local Token token(coroutines_);
  return token;
}

}  // namespace engine::coro

USERVER_NAMESPACE_END
