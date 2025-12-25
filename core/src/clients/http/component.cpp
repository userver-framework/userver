#include <userver/clients/http/component.hpp>

#include <boost/range/adaptor/transformed.hpp>

#include <userver/clients/http/middlewares/component.hpp>

#include <userver/clients/http/middlewares/pipeline_component.hpp>
#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/utils/algo.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

#ifndef ARCADIA_ROOT
#include "generated/src/clients/http/component.yaml.hpp"  // Y_IGNORE
#endif

USERVER_NAMESPACE_BEGIN

namespace components {

namespace {

static std::vector<utils::NotNull<clients::http::MiddlewareBase*>> FindMiddlewares(
    middlewares::impl::MiddlewaresMap middlewares_map,
    const components::ComponentContext& context
) {
    const auto& pipeline_component = context.FindComponent<clients::http::MiddlewarePipelineComponent>();
    for (const auto& [name, config] : pipeline_component.GetMiddlewaresConfig().middlewares) {
        middlewares_map.try_emplace(name, config);
    }
    std::vector<clients::http::middlewares::ComponentBase*> components;
    for (const auto& [name, config] : middlewares_map) {
        if (config.enabled) {
            components.push_back(&context.FindComponent<clients::http::middlewares::ComponentBase>(name));
        }
    }
    std::sort(components.begin(), components.end(), [](const auto& lhs, const auto& rhs) {
        return lhs->GetIndex(utils::impl::InternalTag{}) < rhs->GetIndex(utils::impl::InternalTag{});
    });
    return utils::AsContainer<std::vector<utils::NotNull<
        clients::http::MiddlewareBase*>>>(components | boost::adaptors::transformed([](const auto& component) {
                                              return &component->GetMiddleware();
                                          }));
}

}  // namespace

HttpClient::HttpClient(const ComponentConfig& component_config, const ComponentContext& context)
    : ComponentBase(component_config, context),
      http_client_(
          utils::impl::InternalTag{},
          context
              .FindComponent<components::HttpClientCore>(component_config["core-component"]
                                                             .As<std::string>(components::HttpClientCore::kName))
              .GetHttpClientCore(utils::impl::InternalTag{}),
          FindMiddlewares(component_config["middlewares"].As<middlewares::impl::MiddlewaresMap>({}), context)
      )
{}

clients::http::Client& HttpClient::GetHttpClient() { return http_client_; }

yaml_config::Schema HttpClient::GetStaticConfigSchema() {
    return yaml_config::MergeSchemasFromResource<ComponentBase>("src/clients/http/component.yaml");
}

}  // namespace components

USERVER_NAMESPACE_END
