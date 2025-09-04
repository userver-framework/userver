#include <userver/middlewares/runner.hpp>

#include <fmt/format.h>
#include <fmt/ranges.h>

#include <userver/formats/common/merge.hpp>
#include <userver/formats/yaml/value_builder.hpp>
#include <userver/logging/log.hpp>
#include <userver/yaml_config/impl/validate_static_config.hpp>

#include <userver/middlewares/groups.hpp>

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
    LOG_INFO() << fmt::format("Middlewares configuration for {}: [{}]", component_name, fmt::join(names, ", "));
}

void LogValidateError(std::string_view middleware_name, const std::exception& e) {
    LOG_ERROR()
        << fmt::format(
               "Error while creating the middleware '{}'. Probably, you make a typo in middleware options. Exception: ",
               middleware_name
           )
        << e;
}

}  // namespace middlewares::impl

USERVER_NAMESPACE_END
