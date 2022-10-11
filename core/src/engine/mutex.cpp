#include <userver/engine/mutex.hpp>

#include <engine/impl/mutex_impl.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine {

class Mutex::Impl final : public impl::MutexImpl<impl::WaitList> {};

Mutex::Mutex() = default;

Mutex::~Mutex() = default;

void Mutex::lock() { impl_->lock(); }

void Mutex::unlock() { impl_->unlock(); }

bool Mutex::try_lock() { return impl_->try_lock(); }

bool Mutex::try_lock_until(Deadline deadline) {
  return impl_->try_lock_until(deadline);
}

}  // namespace engine

USERVER_NAMESPACE_END
