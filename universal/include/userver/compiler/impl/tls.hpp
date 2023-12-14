#pragma once

#include <type_traits>
#include <utility>

/// @cond

// This attribute should be put on functions that read or write (or read the
// address of) 'thread_local' variables. (And such functions should never
// contain a context switch themselves.) It should prevent the compiler from
// caching the TLS address across a coroutine context switch.
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define USERVER_IMPL_PREVENT_TLS_CACHING __attribute__((noinline))

// This macro should be put inside functions that read or write (or read the
// address of) 'thread_local' variables. (And such functions should never
// contain a context switch themselves.)
// In conjunction with USERVER_IMPL_PREVENT_TLS_CACHING this prevents the
// compiler from caching the TLS address across a coroutine context switch.
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define USERVER_IMPL_PREVENT_TLS_CACHING_ASM \
  __asm__ volatile("") /* NOLINT(hicpp-no-assembler) */

/// @endcond

USERVER_NAMESPACE_BEGIN

namespace compiler::impl {

/// @brief Each invocation creates a unique thread-local variable that can be
/// used in a coroutine-safe manner.
///
/// The thread-local variable is initialized using the result of the factory.
///
/// The returned variable must not be used across coroutine context switches.
/// That is, after any usage of userver synchronization primitives or clients
/// (web or db), the reference is invalidated and should not be used.
template <typename Func>
USERVER_IMPL_PREVENT_TLS_CACHING auto& ThreadLocal(Func&& factory) {
  using VariableType = std::invoke_result_t<Func&&>;

  thread_local VariableType variable{std::forward<Func>(factory)()};
  VariableType* ptr = &variable;
  // clang-format off
  // NOLINTNEXTLINE(hicpp-no-assembler)
  asm volatile("" : "+rm" (ptr));
  // clang-format on
  return *ptr;
}

}  // namespace compiler::impl

USERVER_NAMESPACE_END
