#include <engine/task/counted_coroutine_ptr.hpp>

#include <engine/task/task_processor.hpp>
#include <engine/task/task_processor_pools.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::impl {

CountedCoroutinePtr::CountedCoroutinePtr(CoroPool::CoroutinePtr coro,
                                         TaskProcessor& task_processor)
    : coro_(std::move(coro)), token_(task_processor.GetTaskCounter()) {}

CountedCoroutinePtr::CoroPool::Coroutine& CountedCoroutinePtr::operator*() {
  UASSERT(coro_);
  return coro_->Get();
}

void CountedCoroutinePtr::ReturnToPool() && {
  if (coro_) std::move(*coro_).ReturnToPool();
  token_ = std::nullopt;
}

}  // namespace engine::impl

USERVER_NAMESPACE_END
