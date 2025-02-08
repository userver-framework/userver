#include <userver/ugrpc/middlewares/runner.hpp>

#include <userver/formats/common/merge.hpp>
#include <userver/formats/yaml/value_builder.hpp>
#include <userver/yaml_config/impl/validate_static_config.hpp>

#include <userver/ugrpc/server/middlewares/groups.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::middlewares::impl {

yaml_config::YamlConfig MakeMiddlewareConfig(
    const formats::yaml::Value& global,
    const yaml_config::YamlConfig& local,
    yaml_config::Schema schema
) {
    formats::yaml::ValueBuilder builder{std::move(global)};

    if (!local.IsMissing()) {
        formats::common::Merge(builder, local.template As<formats::yaml::Value>());
        schema.properties->erase("load-enabled");
        yaml_config::impl::Validate(local, schema);
    }
    return yaml_config::YamlConfig{builder.ExtractValue(), formats::yaml::Value{}};
}

MiddlewareDependencyBuilder MakeDefaultUserDependency() {
    return MiddlewareDependencyBuilder().InGroup<server::groups::User>();
}

}  // namespace ugrpc::middlewares::impl

USERVER_NAMESPACE_END
