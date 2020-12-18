#pragma once

#include <optional>

#include <engine/coro/pool.hpp>
#include <engine/task/task_counter.hpp>
#include <engine/task/task_processor_fwd.hpp>
#include <utils/assert.hpp>

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

  explicit operator bool() const { return !!coro_; }

  uint64_t Id() const { return reinterpret_cast<uint64_t>(coro_.get()); }
  decltype(auto) operator*() {
    UASSERT(coro_);
    return *coro_;
  }

  void ReturnToPool() &&;

 private:
  CoroPool::CoroutinePtr coro_;
  std::optional<TaskCounter::CoroToken> token_;
  CoroPool* coro_pool_{nullptr};
};

}  // namespace engine::impl
