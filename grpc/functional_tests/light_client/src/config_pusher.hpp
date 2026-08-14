#pragma once

#include <string>
#include <vector>

#include <userver/components/component_base.hpp>
#include <userver/components/component_context.hpp>
#include <userver/dynamic_config/updates_sink/component.hpp>
#include <userver/dynamic_config/updates_sink/find.hpp>
#include <userver/dynamic_config/value.hpp>
#include <userver/utest/using_namespace_userver.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

#include <userver/ugrpc/client/client_factory_component.hpp>

namespace functional_tests {

/// Depends on the "light" ClientFactoryComponent(s) named in
/// `factory-components` (mimicking a real service, where light clients are
/// used to deliver the very first successful dynamic config update to the
/// service itself) and, once ALL of them finish constructing, pushes that
/// update. This is this service's ONLY source of dynamic config updates: if
/// ANY of the listed factories ever regressed into blocking on
/// components::DynamicConfig::GetSource() during its own construction, this
/// component would never be able to unblock it, deadlocking the whole
/// service on startup.
class ConfigPusher final : public components::ComponentBase {
public:
    static constexpr std::string_view kName = "config-pusher";

    ConfigPusher(const components::ComponentConfig& config, const components::ComponentContext& context)
        : components::ComponentBase(config, context) {
        for (const auto& factory_component : config["factory-components"].As<std::vector<std::string>>()) {
            context.FindComponent<ugrpc::client::ClientFactoryComponent>(factory_component);
        }
        dynamic_config::FindUpdatesSink(config, context).SetConfig(kName, dynamic_config::DocsMap{});
    }

    static yaml_config::Schema GetStaticConfigSchema() {
        return yaml_config::MergeSchemas<components::ComponentBase>(R"(
type: object
description: >
    Pushes the service's only dynamic config update, after all listed light
    client factories finish constructing.
additionalProperties: false
properties:
    factory-components:
        type: array
        description: names of the ClientFactoryComponent-s to depend on before pushing the update
        items:
            type: string
            description: ClientFactoryComponent name
)");
    }
};

}  // namespace functional_tests
