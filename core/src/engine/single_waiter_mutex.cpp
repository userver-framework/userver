#include <userver/engine/single_waiter_mutex.hpp>

#include <engine/impl/mutex_impl.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine {

struct SingleWaiterMutex::Impl final {
  impl::MutexImpl<impl::WaitListLight> mutex;
};

SingleWaiterMutex::SingleWaiterMutex() {}

SingleWaiterMutex::~SingleWaiterMutex() {}

void SingleWaiterMutex::lock() { impl_->mutex.lock(); }

void SingleWaiterMutex::unlock() { impl_->mutex.unlock(); }

bool SingleWaiterMutex::try_lock() { return impl_->mutex.try_lock(); }

bool SingleWaiterMutex::try_lock_until(Deadline deadline) {
  return impl_->mutex.try_lock_until(deadline);
}

}  // namespace engine

USERVER_NAMESPACE_END
