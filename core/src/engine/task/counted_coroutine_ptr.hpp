#pragma once

#include <optional>

#include <engine/coro/pool.hpp>
#include <engine/task/task_counter.hpp>
#include <utils/assert.hpp>

namespace engine {

class TaskProcessor;

namespace impl {

class TaskContext;

class CountedCoroutinePtr final {
 public:
  using CoroPool = coro::Pool<TaskContext>;

  CountedCoroutinePtr();
  CountedCoroutinePtr(CoroPool::CoroutinePtr, TaskProcessor&);
  ~CountedCoroutinePtr();

  // TODO: noexcept when std::optional
  CountedCoroutinePtr(CountedCoroutinePtr&&);
  CountedCoroutinePtr& operator=(CountedCoroutinePtr&&);

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

}  // namespace impl
}  // namespace engine
