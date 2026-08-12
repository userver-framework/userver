#include <userver/components/component_base.hpp>

#include <userver/components/component.hpp>
#include <userver/engine/deadline.hpp>
#include <userver/logging/component.hpp>
#include <userver/yaml_config/schema.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {

ComponentBase::ComponentBase(const ComponentConfig&, const ComponentContext& component_context) {
    component_context.FindComponent<Logging>();
}

void ComponentBase::OnGracefulShutdown(engine::Deadline) {}

yaml_config::Schema ComponentBase::GetStaticConfigSchema() {
    auto schema = RawComponentBase::GetStaticConfigSchema();
    schema.UpdateDescription("Base class for all application components, it depends on components::Logger.");
    return schema;
}

}  // namespace components

USERVER_NAMESPACE_END
