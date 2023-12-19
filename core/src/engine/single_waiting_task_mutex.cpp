#include <userver/engine/single_waiting_task_mutex.hpp>

#include <engine/impl/mutex_impl.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine {

class SingleWaitingTaskMutex::Impl final
    : public impl::MutexImpl<impl::WaitListLight> {};

SingleWaitingTaskMutex::SingleWaitingTaskMutex() = default;

SingleWaitingTaskMutex::~SingleWaitingTaskMutex() = default;

void SingleWaitingTaskMutex::lock() { impl_->lock(); }

void SingleWaitingTaskMutex::unlock() { impl_->unlock(); }

bool SingleWaitingTaskMutex::try_lock() { return impl_->try_lock(); }

bool SingleWaitingTaskMutex::try_lock_until(Deadline deadline) {
  return impl_->try_lock_until(deadline);
}

}  // namespace engine

USERVER_NAMESPACE_END
