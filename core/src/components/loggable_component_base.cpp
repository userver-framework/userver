#include <userver/components/loggable_component_base.hpp>

#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/components/tracer.hpp>
#include <userver/logging/component.hpp>

namespace components {

LoggableComponentBase::LoggableComponentBase(
    const ComponentConfig&, const ComponentContext& component_context) {
  component_context.FindComponent<Logging>();
  component_context.FindComponent<Tracer>();
}

}  // namespace components
