#include <engine/coro/pool.hpp>

#include <algorithm>  // for std::max/std::min
#include <iterator>
#include <optional>

#include <userver/logging/log.hpp>
#include <userver/utils/assert.hpp>

#include <utils/sys_info.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::coro {

Pool::Pool(PoolConfig config, Executor executor)
    : config_(FixupConfig(std::move(config))),
      executor_(executor),
      local_coroutine_move_size_((config_.local_cache_size + 1) / 2),
      stack_allocator_(config_.stack_size),
      stack_usage_monitor_(config_.stack_size),
      initial_coroutines_(config_.initial_size),
      used_coroutines_(config_.max_size),
      idle_coroutines_num_(config_.initial_size),
      total_coroutines_num_(0) {
  UASSERT(local_coroutine_move_size_ <= config_.local_cache_size);
  moodycamel::ProducerToken token(initial_coroutines_);

  stack_usage_monitor_.Start();

  for (std::size_t i = 0; i < config_.initial_size; ++i) {
    bool ok =
        initial_coroutines_.enqueue(token, CreateCoroutine(/*quiet =*/true));
    UINVARIANT(ok, "Failed to allocate the initial coro pool");
  }
}

Pool::~Pool() = default;

typename Pool::CoroutinePtr Pool::GetCoroutine() {
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
    --idle_coroutines_num_;
  } else {
    coroutine.emplace(CreateCoroutine());
  }

  return CoroutinePtr(std::move(*coroutine), *this);
}

void Pool::PutCoroutine(CoroutinePtr&& coroutine_ptr) {
  if (config_.local_cache_size == 0) {
    const bool ok =
        // We only ever return coroutines into our 'working set'.
        used_coroutines_.enqueue(GetUsedPoolToken<moodycamel::ProducerToken>(),
                                 std::move(coroutine_ptr.Get()));
    if (ok) {
      ++idle_coroutines_num_;
    }
    return;
  }

  if (local_coro_buffer_.size() >= config_.local_cache_size) {
    DepopulateLocalCache();
  }

  local_coro_buffer_.push_back(std::move(coroutine_ptr.Get()));
}

PoolStats Pool::GetStats() const {
  PoolStats stats;
  stats.active_coroutines =
      total_coroutines_num_.load() -
      (used_coroutines_.size_approx() + initial_coroutines_.size_approx());
  stats.total_coroutines =
      std::max(total_coroutines_num_.load(), stats.active_coroutines);
  stats.max_stack_usage_pct = stack_usage_monitor_.GetMaxStackUsagePct();
  stats.is_stack_usage_monitor_active = stack_usage_monitor_.IsActive();
  return stats;
}

void Pool::PrepareLocalCache() {
  local_coro_buffer_.reserve(config_.local_cache_size);
}

void Pool::ClearLocalCache() {
  const std::size_t current_idle_coroutines_num = idle_coroutines_num_.load();
  std::size_t return_to_pool_from_local_cache_num = 0;

  if (current_idle_coroutines_num < config_.max_size) {
    return_to_pool_from_local_cache_num =
        std::min(config_.max_size - current_idle_coroutines_num,
                 local_coro_buffer_.size());

    const bool ok = used_coroutines_.enqueue_bulk(
        GetUsedPoolToken<moodycamel::ProducerToken>(),
        std::make_move_iterator(local_coro_buffer_.begin()),
        return_to_pool_from_local_cache_num);
    if (ok) {
      idle_coroutines_num_.fetch_add(return_to_pool_from_local_cache_num);
    } else {
      return_to_pool_from_local_cache_num = 0;
    }
  }

  total_coroutines_num_ -=
      local_coro_buffer_.size() - return_to_pool_from_local_cache_num;
  local_coro_buffer_.clear();
}

Pool::Coroutine Pool::CreateCoroutine(bool quiet) {
  try {
    Coroutine coroutine(stack_allocator_, executor_);
    const auto new_total = ++total_coroutines_num_;
    if (!quiet) {
      LOG_DEBUG() << "Created a coroutine #" << new_total << '/'
                  << config_.max_size;
    }

    stack_usage_monitor_.Register(coroutine);

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

void Pool::OnCoroutineDestruction() noexcept { --total_coroutines_num_; }

bool Pool::TryPopulateLocalCache() {
  if (local_coroutine_move_size_ == 0) return false;

  const std::size_t dequeued_num = used_coroutines_.try_dequeue_bulk(
      GetUsedPoolToken<moodycamel::ConsumerToken>(),
      std::back_inserter(local_coro_buffer_), local_coroutine_move_size_);
  if (dequeued_num == 0) return false;

  idle_coroutines_num_.fetch_sub(dequeued_num);
  return true;
}

void Pool::DepopulateLocalCache() {
  const std::size_t current_idle_coroutines_num = idle_coroutines_num_.load();
  std::size_t return_to_pool_from_local_cache_num = 0;

  if (current_idle_coroutines_num < config_.max_size) {
    return_to_pool_from_local_cache_num =
        std::min(config_.max_size - current_idle_coroutines_num,
                 local_coroutine_move_size_);

    const bool ok = used_coroutines_.enqueue_bulk(
        GetUsedPoolToken<moodycamel::ProducerToken>(),
        std::make_move_iterator(local_coro_buffer_.end() -
                                return_to_pool_from_local_cache_num),
        return_to_pool_from_local_cache_num);
    if (ok) {
      idle_coroutines_num_.fetch_add(return_to_pool_from_local_cache_num);
    } else {
      return_to_pool_from_local_cache_num = 0;
    }
  }

  total_coroutines_num_ -=
      local_coroutine_move_size_ - return_to_pool_from_local_cache_num;
  local_coro_buffer_.erase(
      local_coro_buffer_.end() - local_coroutine_move_size_,
      local_coro_buffer_.end());
}

std::size_t Pool::GetStackSize() const { return config_.stack_size; }

PoolConfig Pool::FixupConfig(PoolConfig&& config) {
  const auto page_size = utils::sys_info::GetPageSize();
  config.stack_size = (config.stack_size + page_size - 1) & ~(page_size - 1);

  return std::move(config);
}

void Pool::RegisterThread() { stack_usage_monitor_.RegisterThread(); }

void Pool::AccountStackUsage() { stack_usage_monitor_.AccountStackUsage(); }

template <typename Token>
Token& Pool::GetUsedPoolToken() {
  thread_local Token token(used_coroutines_);
  return token;
}

//////////////////////////////////////////////////////////////

Pool::CoroutinePtr::CoroutinePtr(Pool::Coroutine&& coro, Pool& pool) noexcept
    : coro_(std::move(coro)), pool_(&pool) {}

Pool::CoroutinePtr::~CoroutinePtr() {
  UASSERT(pool_);
  if (coro_) pool_->OnCoroutineDestruction();
}

Pool::Coroutine& Pool::CoroutinePtr::Get() noexcept {
  UASSERT(coro_);
  return coro_;
}

void Pool::CoroutinePtr::ReturnToPool() && {
  UASSERT(coro_);
  pool_->PutCoroutine(std::move(*this));
}

}  // namespace engine::coro

USERVER_NAMESPACE_END
