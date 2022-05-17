#pragma once

#include <optional>

#include <engine/coro/pool.hpp>
#include <engine/task/task_counter.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::impl {

class TaskContext;

class CountedCoroutinePtr final {
 public:
  using CoroPool = coro::Pool<TaskContext>;

  CountedCoroutinePtr() = default;
  CountedCoroutinePtr(CoroPool::CoroutinePtr, TaskProcessor&);

  CountedCoroutinePtr(const CountedCoroutinePtr&) = delete;
  CountedCoroutinePtr(CountedCoroutinePtr&&) noexcept = default;
  CountedCoroutinePtr& operator=(const CountedCoroutinePtr&) = delete;
  CountedCoroutinePtr& operator=(CountedCoroutinePtr&&) noexcept = default;

  explicit operator bool() const { return static_cast<bool>(coro_); }

  CoroPool::Coroutine& operator*() {
    UASSERT(coro_);
    return coro_->Get();
  }

  void ReturnToPool() &&;

 private:
  std::optional<CoroPool::CoroutinePtr> coro_;
  std::optional<TaskCounter::CoroToken> token_;
  CoroPool* coro_pool_{nullptr};
};

}  // namespace engine::impl

USERVER_NAMESPACE_END
