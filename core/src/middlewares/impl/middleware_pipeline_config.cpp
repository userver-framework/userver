#include <userver/middlewares/impl/middleware_pipeline_config.hpp>

#include <userver/yaml_config/yaml_config.hpp>

USERVER_NAMESPACE_BEGIN

namespace middlewares::impl {

MiddlewaresConfig Parse(const yaml_config::YamlConfig& value, formats::parse::To<MiddlewaresConfig>) {
    MiddlewaresConfig config;
    config.middlewares = value["middlewares"].As<decltype(config.middlewares)>({});
    return config;
}

BaseMiddlewareConfig Parse(const yaml_config::YamlConfig& value, formats::parse::To<BaseMiddlewareConfig>) {
    BaseMiddlewareConfig config{};
    config.enabled = value["enabled"].As<bool>(config.enabled);
    return config;
}

MiddlewareRunnerConfig Parse(const yaml_config::YamlConfig& value, formats::parse::To<MiddlewareRunnerConfig>) {
    MiddlewareRunnerConfig conf{};
    conf.disable_user_group = value["disable-user-pipeline-middlewares"].As<bool>(conf.disable_user_group);
    conf.disable_all = value["disable-all-pipeline-middlewares"].As<bool>(conf.disable_all);
    conf.middlewares = value["middlewares"].As<std::unordered_map<std::string, yaml_config::YamlConfig>>({});
    return conf;
}

bool operator==(const MiddlewareEnabled& l, const MiddlewareEnabled& r) {
    return l.name == r.name && l.enabled == r.enabled;
}

}  // namespace middlewares::impl

USERVER_NAMESPACE_END
