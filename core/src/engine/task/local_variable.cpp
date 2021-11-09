#include <userver/engine/task/local_variable.hpp>

#include <engine/task/task_context.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::impl {

TaskLocalVariableAny::TaskLocalVariableAny()
    : variable_index_(LocalStorage::RegisterVariable()) {}

std::size_t TaskLocalVariableAny::GetVariableIndex() const {
  return variable_index_;
}

impl::LocalStorage& GetCurrentLocalStorage() {
  return current_task::GetCurrentTaskContext().GetLocalStorage();
}

}  // namespace engine::impl

USERVER_NAMESPACE_END
