#include <userver/clients/http/plugins/retry_budget/component.hpp>

#include <clients/http/plugins/retry_budget/plugin.hpp>

#include <userver/components/component_config.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

USERVER_NAMESPACE_BEGIN

namespace clients::http::plugins::retry_budget {

Component::Component(const components::ComponentConfig& config, const components::ComponentContext& context)
    : ComponentBase(config, context), plugin_(std::make_unique<retry_budget::Plugin>()) {
    auto policy = config.As<USERVER_NAMESPACE::utils::RetryBudgetSettings>();
    plugin_->Storage().SetPolicy(policy);
}

Component::~Component() = default;

http::Plugin& Component::GetPlugin() { return *plugin_; }

yaml_config::Schema Component::GetStaticConfigSchema() {
    return yaml_config::MergeSchemas<components::ComponentBase>(R"(
type: object
description: Component to limit too active requests, also known as CC.
additionalProperties: false
properties:
    max-tokens:
        type: number
        description: max amount of tokens in the token bucket
        defaultDescription: 10
    token-ratio:
        type: number
        description: retry count to successful responses ratio
        defaultDescription: 0.1
)");
}

}  // namespace clients::http::plugins::retry_budget

USERVER_NAMESPACE_END
