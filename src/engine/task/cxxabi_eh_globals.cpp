#include "cxxabi_eh_globals.hpp"

#include <cxxabi.h>

#include <engine/task/task_context.hpp>

namespace engine {
namespace impl {

namespace {

abi::__cxa_eh_globals* GetGlobals() throw() {
  thread_local EhGlobals tls_globals;
  auto* globals = &tls_globals;

  auto* context = current_task::GetCurrentTaskContextUnchecked();
  if (context) globals = context->GetEhGlobals();

  return reinterpret_cast<abi::__cxa_eh_globals*>(globals);
}

extern "C" abi::__cxa_eh_globals* __cxa_get_globals() throw() {
  return GetGlobals();
}
extern "C" abi::__cxa_eh_globals* __cxa_get_globals_fast() throw() {
  return GetGlobals();
}

}  // namespace
}  // namespace impl
}  // namespace engine
