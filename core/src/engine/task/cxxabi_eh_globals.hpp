#pragma once

#include <cxxabi.h>
#include <cstring>

#include <sys/param.h>

USERVER_NAMESPACE_BEGIN

#if defined(__linux__) && defined(__GLIBCXX__)

#define USERVER_EHGLOBALS_INTERPOSE

namespace engine::impl {

// This structure is a __cxa_eh_globals storage for coroutines.
// It is required to allow task switching during unwind.
struct EhGlobals {
  void* data[4];
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
  EhGlobals() noexcept { ::memset(data, 0, sizeof(data)); }
};

// NOLINTNEXTLINE(hicpp-use-noexcept,modernize-use-noexcept)
abi::__cxa_eh_globals* GetGlobals() throw();

}  // namespace engine::impl

#elif defined(_LIBCPP_VERSION) && \
    (defined(__APPLE__) || defined(BSD) || defined(Y_CXA_EH_GLOBALS_COMPLETE))

// MAC_COMPAT, YA_COMPAT
// "only INSERTED libraries can interpose", you say
// duplicate symbols, you say
// let's juggle some razor blades then
#define USERVER_EHGLOBALS_SWAP

// NOLINTNEXTLINE(bugprone-reserved-identifier,cert-dcl37-c,cert-dcl51-cpp)
namespace __cxxabiv1 {
// NOLINTNEXTLINE(bugprone-reserved-identifier,cert-dcl37-c,cert-dcl51-cpp)
struct __cxa_exception;
}  // namespace __cxxabiv1

namespace engine::impl {

struct EhGlobals {
  __cxxabiv1::__cxa_exception* caught_exceptions{nullptr};
  unsigned int uncaught_exceptions{0};
};

}  // namespace engine::impl

#else
#error "We don't support exceptions in your environment"
#endif

namespace engine::impl {
void ExchangeEhGlobals(EhGlobals&) noexcept;
}  // namespace engine::impl

USERVER_NAMESPACE_END
