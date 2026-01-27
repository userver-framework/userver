#include <userver/ugrpc/server/middlewares/headers_propagator/component.hpp>

#include <boost/range/adaptor/transformed.hpp>

#include <userver/components/component_config.hpp>
#include <userver/utils/algo.hpp>
#include <userver/utils/text.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

#include <ugrpc/server/middlewares/headers_propagator/middleware.hpp>

#ifndef ARCADIA_ROOT
#include "generated/src/ugrpc/server/middlewares/headers_propagator/component.yaml.hpp"  // Y_IGNORE
#endif

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server::middlewares::headers_propagator {

Component::Component(const components::ComponentConfig& config, const components::ComponentContext& context)
    : MiddlewareFactoryComponentBase(config, context)
{}

std::shared_ptr<const MiddlewareBase> Component::CreateMiddleware(
    const ugrpc::server::ServiceInfo&,
    const yaml_config::YamlConfig& middleware_config
) const {
    const auto headers = middleware_config["headers"].As<std::vector<std::string>>({});
    return std::make_shared<
        Middleware>(utils::AsContainer<decltype(headers
                    )>(headers | boost::adaptors::transformed([](const auto& h) { return utils::text::ToLower(h); })));
}

yaml_config::Schema Component::GetMiddlewareConfigSchema() const { return GetStaticConfigSchema(); }

yaml_config::Schema Component::GetStaticConfigSchema() {
    return yaml_config::MergeSchemasFromResource<
        MiddlewareFactoryComponentBase>("src/ugrpc/server/middlewares/headers_propagator/component.yaml");
}

}  // namespace ugrpc::server::middlewares::headers_propagator

USERVER_NAMESPACE_END
