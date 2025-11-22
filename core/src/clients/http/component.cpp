#include <userver/clients/http/component.hpp>

#include <boost/range/adaptor/map.hpp>

#include <userver/clients/http/plugin_component.hpp>

#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/utils/algo.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {

namespace {

constexpr std::string_view kHttpClientPluginPrefix = "http-client-plugin-";

using PluginsIndices = std::unordered_map<std::string, std::uint32_t>;

static std::vector<utils::NotNull<clients::http::Plugin*>>
FindPlugins(const PluginsIndices& plugins_indices, const components::ComponentContext& context) {
    auto names = utils::AsContainer<std::vector<std::string>>(plugins_indices | boost::adaptors::map_keys);
    std::sort(names.begin(), names.end(), [&plugins_indices](const std::string& lhs, const std::string& rhs) {
        return std::tie(plugins_indices.at(lhs), lhs) < std::tie(plugins_indices.at(rhs), rhs);
    });
    std::vector<utils::NotNull<clients::http::Plugin*>> plugins;
    for (const auto& name : names) {
        auto& component =
            context.FindComponent<clients::http::plugin::ComponentBase>(std::string{kHttpClientPluginPrefix} + name);
        plugins.emplace_back(&component.GetPlugin());
    }
    return plugins;
}

}  // namespace

HttpClient::HttpClient(const ComponentConfig& component_config, const ComponentContext& context)
    : ComponentBase(component_config, context),
      http_client_(
          utils::impl::InternalTag{},
          context
              .FindComponent<components::HttpClientCore>(
                  component_config["core-component"].As<std::string>(components::HttpClientCore::kName)
              )
              .GetHttpClientCore(utils::impl::InternalTag{}),
          FindPlugins(component_config["plugins"].As<PluginsIndices>({}), context)
      ) {}

clients::http::Client& HttpClient::GetHttpClient() { return http_client_; }

yaml_config::Schema HttpClient::GetStaticConfigSchema() {
    return yaml_config::MergeSchemas<ComponentBase>(R"(
type: object
description: Component that manages clients::http::ClientWithPlugins.
additionalProperties: false
properties:
    core-component:
        type: string
        description: name of HttpClientCore component to use
        defaultDescription: http-client-core
    plugins:
        type: object
        description: HTTP client plugins
        properties: {}
        additionalProperties:
            type: integer
            description: Index by which plugins are sorted
            minimum: 0
)");
}

}  // namespace components

USERVER_NAMESPACE_END
