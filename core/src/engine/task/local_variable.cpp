#include <userver/engine/task/local_variable.hpp>

#include <engine/task/task_context.hpp>

namespace engine {

impl::LocalStorage& TaskLocalVariableAny::GetCurrentLocalStorage() {
  return current_task::GetCurrentTaskContext()->GetLocalStorage();
}

TaskLocalVariableAny::TaskLocalVariableAny()
    : coro_variable_index_(impl::LocalStorage::RegisterVariable()) {}

}  // namespace engine
