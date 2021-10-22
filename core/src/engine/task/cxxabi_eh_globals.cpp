#include <engine/task/cxxabi_eh_globals.hpp>

#include <cxxabi.h>
#include <cstring>

#include <engine/task/task_context.hpp>

USERVER_NAMESPACE_BEGIN

#if defined(USERVER_EHGLOBALS_INTERPOSE)

namespace engine::impl {

// NOLINTNEXTLINE(hicpp-use-noexcept,modernize-use-noexcept)
abi::__cxa_eh_globals* GetGlobals() throw() {
  thread_local EhGlobals tls_globals;
  auto* globals = &tls_globals;

  auto* context = current_task::GetCurrentTaskContextUnchecked();
  if (context) globals = context->GetEhGlobals();

  return reinterpret_cast<abi::__cxa_eh_globals*>(globals);
}

void ExchangeEhGlobals(EhGlobals&) noexcept {
  // We process all requests ourselves already, no need to do anything
}

}  // namespace engine::impl

#elif defined(USERVER_EHGLOBALS_SWAP)

namespace __cxxabiv1 {
struct __cxa_eh_globals;
}  // namespace __cxxabiv1

extern "C" __cxxabiv1::__cxa_eh_globals* __cxa_get_globals();

namespace engine::impl {

void ExchangeEhGlobals(EhGlobals& replacement) noexcept {
  auto* current = __cxa_get_globals();

  EhGlobals buf;
  std::memcpy(&buf, current, sizeof(EhGlobals));
  std::memcpy(current, &replacement, sizeof(EhGlobals));
  std::memcpy(&replacement, &buf, sizeof(EhGlobals));
}

}  // namespace engine::impl

#else
#error "No eh_globals policy selected"
#endif

USERVER_NAMESPACE_END
