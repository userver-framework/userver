#pragma once

#include <cstddef>

#include <userver/engine/task/local_storage.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine {

namespace impl {

class TaskLocalVariableAny final {
 public:
  TaskLocalVariableAny();

  TaskLocalVariableAny(const TaskLocalVariableAny&) = delete;
  TaskLocalVariableAny(TaskLocalVariableAny&&) = delete;
  TaskLocalVariableAny& operator=(const TaskLocalVariableAny&) = delete;
  TaskLocalVariableAny& operator=(TaskLocalVariableAny&&) = delete;

  std::size_t GetVariableIndex() const;

 private:
  const std::size_t variable_index_;
};

impl::LocalStorage& GetCurrentLocalStorage();

}  // namespace impl

/** @brief TaskLocalVariable is a per-coroutine variable of arbitrary type.
 *
 * It is an alternative to `thread_local`, but per-task instead of per-thread.
 *
 * The order of destruction of task-local variables is inverse to the order of
 * initialization.
 *
 * @note Currently `T` must not be an engine type, i.e. it must not call any
 * coroutine-specific code from `~T` as `~T` is called outside of any coroutine.
 */
template <typename T>
class TaskLocalVariable final {
 public:
  /// @brief Get the instance of the variable for the current coroutine.
  /// @note Must be called from a coroutine, otherwise it is UB.
  T& operator*();

  T* operator->();

 private:
  impl::TaskLocalVariableAny impl_;
};

template <typename T>
T& TaskLocalVariable<T>::operator*() {
  return impl::GetCurrentLocalStorage().Get<T>(impl_.GetVariableIndex());
}

template <typename T>
T* TaskLocalVariable<T>::operator->() {
  return &(**this);
}

}  // namespace engine

USERVER_NAMESPACE_END
