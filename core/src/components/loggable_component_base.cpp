#include <userver/components/loggable_component_base.hpp>

#include <userver/components/component.hpp>
#include <userver/components/tracer.hpp>
#include <userver/logging/component.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {

LoggableComponentBase::LoggableComponentBase(
    const ComponentConfig&, const ComponentContext& component_context) {
  component_context.FindComponent<Logging>();
  component_context.FindComponent<Tracer>();
}

}  // namespace components

USERVER_NAMESPACE_END
