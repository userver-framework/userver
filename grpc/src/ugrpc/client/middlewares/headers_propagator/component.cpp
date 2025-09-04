#include <userver/ugrpc/client/middlewares/headers_propagator/component.hpp>

#include <userver/components/component_config.hpp>
#include <userver/formats/parse/common_containers.hpp>
#include <userver/utils/text.hpp>

#include <ugrpc/client/middlewares/headers_propagator/middleware.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::middlewares::headers_propagator {

Settings Parse(const yaml_config::YamlConfig& config, formats::parse::To<Settings>) {
    Settings settings;
    auto headers = config["skip-headers"].As<std::unordered_map<std::string, bool>>({});
    for (auto&& [header, disabled] : headers) {
        UINVARIANT(disabled, "Only 'true' values are allowed in 'skip-headers'");
        settings.skip_headers.insert(utils::text::ToLower(header));
    }
    return settings;
}

Component::Component(const components::ComponentConfig& config, const components::ComponentContext& context)
    : MiddlewareFactoryComponentBase(config, context) {}

Component::~Component() = default;

std::shared_ptr<const MiddlewareBase> Component::CreateMiddleware(
    const ugrpc::client::ClientInfo& /*client_info*/,
    const yaml_config::YamlConfig& middleware_config
) const {
    return std::make_shared<Middleware>(middleware_config.As<Settings>());
}

yaml_config::Schema Component::GetMiddlewareConfigSchema() const { return GetStaticConfigSchema(); }

yaml_config::Schema Component::GetStaticConfigSchema() {
    return yaml_config::MergeSchemas<MiddlewareFactoryComponentBase>(R"(
type: object
description: gRPC service headers propagator component
additionalProperties: false
properties:
    skip-headers:
        type: object
        description: map from metadata fields (headers) to whether it should be skipped
        defaultDescription: '{}'
        additionalProperties:
            type: boolean
            description: true - disable header
        properties: {}
)");
}

}  // namespace ugrpc::client::middlewares::headers_propagator

USERVER_NAMESPACE_END
