#pragma once

#include <type_traits>
#include <utility>

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
__attribute__((noinline)) auto& ThreadLocal(Func&& factory) {
  // Implementation note: this 'asm' and 'noinline' prevent the compiler
  // from caching the TLS address across a coroutine context switch.
  // The general approach is taken from:
  // https://github.com/qemu/qemu/blob/stable-8.2/include/qemu/coroutine-tls.h#L153
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
