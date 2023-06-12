#pragma once

/// @file userver/engine/task/inherited_variable.hpp
/// @brief @copybrief engine::TaskInheritedVariable

#include <type_traits>
#include <utility>

#include <userver/engine/impl/task_local_storage.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine {

/// @ingroup userver_concurrency
///
/// @brief TaskInheritedVariable is a per-coroutine variable of arbitrary type.
///
/// These are like engine::TaskLocalVariable, but the variable instances are
/// inherited by child tasks created via utils::Async.
///
/// The order of destruction of task-inherited variables is unspecified.
template <typename T>
class TaskInheritedVariable final {
  static_assert(!std::is_reference_v<T>);
  static_assert(!std::is_const_v<T>);

 public:
  /// @brief Get the variable instance for the current task.
  /// @returns the variable or `nullptr` if variable was not set.
  const T* GetOptional() const noexcept {
    return Storage().GetOptional<T, kVariableKind>(impl_.GetKey());
  }

  /// @brief Get the variable instance for the current task.
  /// @throws std::runtime_error if variable was not set.
  const T& Get() const {
    return Storage().Get<T, kVariableKind>(impl_.GetKey());
  }

  /// @brief Sets or replaces the T variable instance.
  template <typename... Args>
  void Emplace(Args&&... args) {
    Storage().Emplace<T, kVariableKind>(impl_.GetKey(),
                                        std::forward<Args>(args)...);
  }

  /// @overload
  void Set(T&& value) { Emplace(std::move(value)); }

  /// @overload
  void Set(const T& value) { Emplace(value); }

  /// @brief Hide the variable so that it is no longer accessible from the
  /// current or new child tasks.
  /// @note The variable might not actually be destroyed immediately.
  void Erase() { Storage().Erase<T, kVariableKind>(impl_.GetKey()); }

 private:
  static constexpr auto kVariableKind =
      impl::task_local::VariableKind::kInherited;

  static impl::task_local::Storage& Storage() noexcept {
    return impl::task_local::GetCurrentStorage();
  }

  impl::task_local::Variable impl_;
};

}  // namespace engine

USERVER_NAMESPACE_END
