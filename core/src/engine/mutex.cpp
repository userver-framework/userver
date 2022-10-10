#include <userver/engine/mutex.hpp>

#include <engine/impl/mutex_impl.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine {

class Mutex::ImplWrapper final {
public:
  using Impl = impl::MutexImpl<impl::WaitList>;

  Impl& operator->() {return impl;};
  Impl& operator*() {return impl;};

private:
  Impl impl;
};

Mutex::Mutex() = default;

Mutex::~Mutex() = default;

void Mutex::lock() { (**impl_wrapper_).lock(); }

void Mutex::unlock() { (**impl_wrapper_).unlock(); }

bool Mutex::try_lock() { return (**impl_wrapper_).try_lock(); }

bool Mutex::try_lock_until(Deadline deadline) {
  return (**impl_wrapper_).try_lock_until(deadline);
}

}  // namespace engine

USERVER_NAMESPACE_END
