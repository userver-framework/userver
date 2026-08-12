#include <userver/middlewares/runner.hpp>

#include <fmt/format.h>
#include <fmt/ranges.h>

#include <userver/formats/common/merge.hpp>
#include <userver/formats/yaml/value_builder.hpp>
#include <userver/logging/log.hpp>
#include <userver/yaml_config/impl/validate_static_config.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

#include <userver/middlewares/groups.hpp>

#ifndef ARCADIA_ROOT
#include "generated/src/middlewares/factory_component_base.yaml.hpp"  // Y_IGNORE
#include "generated/src/middlewares/runner_component_base.yaml.hpp"   // Y_IGNORE
#endif

USERVER_NAMESPACE_BEGIN

namespace middlewares::impl {

yaml_config::YamlConfig ValidateAndMergeMiddlewareConfigs(
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
    return MiddlewareDependencyBuilder().InGroup<groups::User>();
}

void LogConfiguration(std::string_view component_name, const std::vector<std::string>& names) {
    LOG_INFO("Middlewares configuration for {}: [{}]", component_name, fmt::join(names, ", "));
}

void LogValidateError(std::string_view middleware_name, const std::exception& e) {
    LOG_ERROR()
        << fmt::format(
               "Error while creating the middleware '{}'. Probably, you make a typo in middleware options. Exception: ",
               middleware_name
           )
        << e;
}

yaml_config::Schema GetMiddlewareFactoryComponentBaseSchema() {
    return yaml_config::MergeSchemasFromResource<
        components::ComponentBase>("src/middlewares/factory_component_base.yaml");
}

yaml_config::Schema GetRunnerComponentBaseSchema() {
    return yaml_config::MergeSchemasFromResource<components::ComponentBase>("src/middlewares/runner_component_base.yaml"
    );
}

}  // namespace middlewares::impl

USERVER_NAMESPACE_END
