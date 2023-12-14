#include <userver/compiler/thread_local.hpp>

#include <thread>

USERVER_NAMESPACE_BEGIN

namespace compiler::impl {

USERVER_IMPL_PREVENT_TLS_CACHING std::uintptr_t GetCurrentThreadId() noexcept {
  USERVER_IMPL_PREVENT_TLS_CACHING_ASM;
  return reinterpret_cast<std::uintptr_t>(pthread_self());
}

}  // namespace compiler::impl

USERVER_NAMESPACE_END
