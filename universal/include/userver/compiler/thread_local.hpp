#pragma once

/// @file userver/compiler/thread_local.hpp
/// @brief Utilities for using thread-local variables in a coroutine-safe way

#include <type_traits>

#include <userver/compiler/impl/constexpr.hpp>
#include <userver/compiler/impl/lifetime.hpp>
#include <userver/compiler/impl/tls.hpp>

#if __cplusplus >= 202002L && \
    (__clang_major__ >= 13 || !defined(__clang__) && __GNUC__ >= 9)
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define USERVER_IMPL_UNEVALUATED_LAMBDAS
#endif

USERVER_NAMESPACE_BEGIN

namespace compiler {

namespace impl {

bool AreCoroutineSwitchesAllowed() noexcept;

void IncrementLocalCoroutineSwitchBans() noexcept;

void DecrementLocalCoroutineSwitchBans() noexcept;

#ifdef USERVER_IMPL_UNEVALUATED_LAMBDAS
template <typename T, typename Factory = decltype([] { return T{}; })>
using UniqueDefaultFactory = Factory;
#else
template <typename T>
struct UniqueDefaultFactory final {
  static_assert(!sizeof(T),
                "Defaulted syntax for compiler::ThreadLocal is unavailable on "
                "your compiler. Please use the lambda-factory syntax, see the "
                "documentation for compiler::ThreadLocal.");
};
#endif

}  // namespace impl

/// @brief The scope that grants access to a thread-local variable.
///
/// @see compiler::ThreadLocal
template <typename VariableType>
class ThreadLocalScope final {
 public:
  ThreadLocalScope(ThreadLocalScope&&) = delete;
  ThreadLocalScope& operator=(ThreadLocalScope&&) = delete;
  ~ThreadLocalScope();

  /// Access the thread-local variable.
  VariableType& operator*() & noexcept USERVER_IMPL_LIFETIME_BOUND;

  /// Access the thread-local variable.
  VariableType* operator->() & noexcept USERVER_IMPL_LIFETIME_BOUND;

  /// @cond
  explicit ThreadLocalScope(VariableType& variable) noexcept;

  // Store ThreadLocalScope to a variable before using.
  VariableType& operator*() && noexcept = delete;

  // Store ThreadLocalScope to a variable before using.
  VariableType* operator->() && noexcept = delete;
  /// @endcond

 private:
  static_assert(!std::is_reference_v<VariableType>);
  static_assert(!std::is_const_v<VariableType>);

  VariableType& variable_;
};

/// @brief Creates a unique thread-local variable that can be used
/// in a coroutine-safe manner.
///
/// Thread-local variables are known to cause issues when used together
/// with userver coroutines:
///
/// * https://github.com/userver-framework/userver/issues/235
/// * https://github.com/userver-framework/userver/issues/242
///
/// Thread-local variables created through this class are protected against
/// these issues.
///
/// Example usage:
///
/// @snippet compiler/thread_local_test.cpp  sample definition
/// @snippet compiler/thread_local_test.cpp  sample
///
/// The thread-local variable is value-initialized.
///
/// In C++17 mode, or if you need to initialize the variable with some
/// arguments, the ThreadLocal should be passed a capture-less lambda that
/// constructs the variable. Example:
///
/// @snippet compiler/thread_local_test.cpp  sample factory
///
/// Once acquired through @a Use, the reference to the thread-local variable
/// should not be returned or otherwise escape the scope
/// of the ThreadLocalScope object. An example of buggy code:
///
/// @code
/// compiler::ThreadLocal<std::string> local_buffer;
///
/// std::string_view PrepareBuffer(std::string_view x, std::string_view y) {
///   const auto buffer = local_buffer.Use();
///   buffer->clear();
///   buffer->append(x);
///   buffer->append(y);
///   return *buffer;  // <- BUG! Reference to thread-local escapes the scope
/// }
/// @endcode
///
/// Do not store a reference to the thread-local object in a separate variable:
///
/// @code
/// compiler::ThreadLocal<std::string> local_buffer;
///
/// void WriteMessage(std::string_view x, std::string_view y) {
///   auto buffer_scope = local_buffer.Use();
///   // Code smell! This makes it more difficult to see that `buffer` is
///   // a reference to thread-local at a glance.
///   auto& buffer = *buffer_scope;
///   buffer.clear();
///   // ...
/// }
/// @endcode
///
/// Until the variable name goes out of scope, userver engine synchronization
/// primitives and clients (web or db) should not be used.
template <typename VariableType,
          typename Factory = impl::UniqueDefaultFactory<VariableType>>
class ThreadLocal final {
  static_assert(std::is_empty_v<Factory>);
  static_assert(
      std::is_same_v<VariableType, std::invoke_result_t<const Factory&>>);

 public:
  USERVER_IMPL_CONSTEVAL ThreadLocal() : factory_(Factory{}) {}

  USERVER_IMPL_CONSTEVAL /*implicit*/ ThreadLocal(Factory factory)
      : factory_(factory) {}

  ThreadLocalScope<VariableType> Use() {
    return ThreadLocalScope<VariableType>(impl::ThreadLocal(factory_));
  }

 private:
  // The ThreadLocal instance should have static storage duration. Still, if a
  // user defines it as a local variable or even a thread_local variable, it
  // should be harmless in practice, because ThreadLocal is an empty type,
  // mainly used to store the `FactoryFunc` template parameter.
  Factory factory_;
};

template <typename Factory>
ThreadLocal(Factory factory)
    -> ThreadLocal<std::invoke_result_t<const Factory&>, Factory>;

template <typename VariableType>
ThreadLocalScope<VariableType>::ThreadLocalScope(
    VariableType& variable) noexcept
    : variable_(variable) {
#ifndef NDEBUG
  impl::IncrementLocalCoroutineSwitchBans();
#endif
}

template <typename VariableType>
ThreadLocalScope<VariableType>::~ThreadLocalScope() {
#ifndef NDEBUG
  impl::DecrementLocalCoroutineSwitchBans();
#endif
}

template <typename VariableType>
VariableType& ThreadLocalScope<VariableType>::operator*() & noexcept  //
    USERVER_IMPL_LIFETIME_BOUND {
  return variable_;
}

template <typename VariableType>
VariableType* ThreadLocalScope<VariableType>::operator->() & noexcept  //
    USERVER_IMPL_LIFETIME_BOUND {
  return &**this;
}

}  // namespace compiler

USERVER_NAMESPACE_END
