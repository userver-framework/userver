#pragma once

/// @file userver/engine/task/local_variable.hpp
/// @brief @copybrief engine::TaskLocalVariable

#include <type_traits>

#include <userver/engine/impl/task_local_storage.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine {

/// @ingroup userver_concurrency
///
/// @brief TaskLocalVariable is a per-coroutine variable of arbitrary type.
///
/// It is an alternative to `thread_local`, but per-task instead of per-thread.
///
/// The order of destruction of task-local variables is inverse to the order of
/// initialization.
template <typename T>
class TaskLocalVariable final {
  static_assert(!std::is_reference_v<T>);
  static_assert(!std::is_const_v<T>);

 public:
  /// @brief Get the instance of the variable for the current coroutine.
  /// Initializes (default constructs) the variable if it was not previously
  /// initialized.
  /// @note Must be called from a coroutine, otherwise it is UB.
  T& operator*();

  /// @overload
  T* operator->();

  /// @brief Get the variable instance for the current task.
  /// @returns the variable or `nullptr` if variable was not initialized.
  const T* GetOptional() const noexcept {
    return impl::task_local::GetCurrentStorage().GetOptional<T, kVariableKind>(
        impl_.GetKey());
  }

 private:
  static constexpr auto kVariableKind = impl::task_local::VariableKind::kNormal;

  impl::task_local::Variable impl_;
};

template <typename T>
T& TaskLocalVariable<T>::operator*() {
  return impl::task_local::GetCurrentStorage().GetOrEmplace<T, kVariableKind>(
      impl_.GetKey());
}

template <typename T>
T* TaskLocalVariable<T>::operator->() {
  return &(**this);
}

}  // namespace engine

USERVER_NAMESPACE_END
