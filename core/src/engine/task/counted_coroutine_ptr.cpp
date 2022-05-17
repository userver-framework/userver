#include <engine/task/counted_coroutine_ptr.hpp>
#include <engine/task/task_context.hpp>
#include <engine/task/task_processor.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::impl {

CountedCoroutinePtr::CountedCoroutinePtr(CoroPool::CoroutinePtr coro,
                                         TaskProcessor& task_processor)
    : coro_(std::move(coro)),
      token_(task_processor.GetTaskCounter()),
      coro_pool_(&task_processor.GetTaskProcessorPools()->GetCoroPool()) {}

void CountedCoroutinePtr::ReturnToPool() && {
  UASSERT(coro_);
  UASSERT(coro_pool_);
  if (coro_ && coro_pool_) coro_pool_->PutCoroutine(std::move(*coro_));
  token_ = std::nullopt;
}

}  // namespace engine::impl

USERVER_NAMESPACE_END
