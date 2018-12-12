#include <components/component_base.hpp>

#include <components/component_config.hpp>
#include <components/component_context.hpp>
#include <components/tracer.hpp>
#include <logging/component.hpp>

namespace components {

LoggableComponentBase::LoggableComponentBase(
    const ComponentConfig&, const ComponentContext& component_context) {
  component_context.FindComponent<Logging>();
  component_context.FindComponent<Tracer>();
}

}  // namespace components
