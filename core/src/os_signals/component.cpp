#include <userver/os_signals/component.hpp>

#include <userver/engine/task/task_base.hpp>

#include <utils/internal_tag.hpp>

USERVER_NAMESPACE_BEGIN

namespace os_signals {

ProcessorComponent::ProcessorComponent(const components::ComponentConfig&,
                                       const components::ComponentContext&)
    : manager_(engine::current_task::GetTaskProcessor()) {}

os_signals::Processor& ProcessorComponent::Get() { return manager_; }

}  // namespace os_signals

USERVER_NAMESPACE_END
