#include <engine/task/local_variable.hpp>

#include <engine/task/task_context.hpp>

namespace engine {

engine::LocalStorage& TaskLocalVariableAny::GetCurrentLocalStorage() {
  return engine::current_task::GetCurrentTaskContext()->GetLocalStorage();
}

TaskLocalVariableAny::TaskLocalVariableAny()
    : coro_variable_index_(engine::LocalStorage::RegisterVariable()) {}

}  // namespace engine
