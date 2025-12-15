#include <userver/clients/http/middlewares/retry_budget/component.hpp>

#include <clients/http/middlewares/retry_budget/middleware.hpp>

#include <userver/components/component_config.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

#ifndef ARCADIA_ROOT
#include "generated/src/clients/http/middlewares/retry_budget/component.yaml.hpp"  // Y_IGNORE
#endif

USERVER_NAMESPACE_BEGIN

namespace clients::http::middlewares::retry_budget {

Component::Component(const components::ComponentConfig& config, const components::ComponentContext& context)
    : ComponentBase(config, context, MiddlewareIndex{2}),
      middleware_(std::make_unique<retry_budget::Middleware>())
{
    auto policy = config.As<USERVER_NAMESPACE::utils::RetryBudgetSettings>();
    middleware_->Storage().SetPolicy(policy);
}

Component::~Component() = default;

http::MiddlewareBase& Component::GetMiddleware() { return *middleware_; }

yaml_config::Schema Component::GetStaticConfigSchema() {
    return yaml_config::MergeSchemasFromResource<
        components::ComponentBase>("src/clients/http/middlewares/retry_budget/component.yaml");
}

}  // namespace clients::http::middlewares::retry_budget

USERVER_NAMESPACE_END
