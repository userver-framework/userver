#include <userver/engine/single_waiter_mutex.hpp>

#include <engine/impl/mutex_impl.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine {

class SingleWaiterMutex::ImplWrapper final {
public:
  using Impl = impl::MutexImpl<impl::WaitListLight>;

  Impl& operator->() {return impl;};
  Impl& operator*() {return impl;};
private:
  Impl impl;
};

SingleWaiterMutex::SingleWaiterMutex() = default;

SingleWaiterMutex::~SingleWaiterMutex() = default;

void SingleWaiterMutex::lock() { (**impl_wrapper_).lock(); }

void SingleWaiterMutex::unlock() { (**impl_wrapper_).unlock(); }

bool SingleWaiterMutex::try_lock() { return (**impl_wrapper_).try_lock(); }

bool SingleWaiterMutex::try_lock_until(Deadline deadline) {
  return (**impl_wrapper_).try_lock_until(deadline);
}

}  // namespace engine

USERVER_NAMESPACE_END
