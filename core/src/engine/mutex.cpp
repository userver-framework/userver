#include <userver/engine/mutex.hpp>

#include <engine/impl/mutex_impl.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine {

struct Mutex::Impl final {
  impl::MutexImpl<impl::WaitList> mutex;
};

Mutex::Mutex() = default;

Mutex::~Mutex() = default;

void Mutex::lock() { impl_->mutex.lock(); }

void Mutex::unlock() { impl_->mutex.unlock(); }

bool Mutex::try_lock() { return impl_->mutex.try_lock(); }

bool Mutex::try_lock_until(Deadline deadline) {
  return impl_->mutex.try_lock_until(deadline);
}

}  // namespace engine

USERVER_NAMESPACE_END
