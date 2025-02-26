#include <userver/ugrpc/impl/middleware_pipeline_config.hpp>

#include <userver/yaml_config/yaml_config.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::middlewares::impl {

MiddlewarePipelineConfig Parse(const yaml_config::YamlConfig& value, formats::parse::To<MiddlewarePipelineConfig>) {
    MiddlewarePipelineConfig config;
    config.middlewares = value["middlewares"].As<std::unordered_map<std::string, BaseMiddlewareConfig>>({});
    return config;
}

const std::unordered_map<std::string, BaseMiddlewareConfig>& UserverMiddlewares() {
    static std::unordered_map<std::string, BaseMiddlewareConfig> core_pipeline{
        {"grpc-server-logging", {}},
        {"grpc-server-baggage", {}},
        {"grpc-server-congestion-control", {}},
        {"grpc-server-deadline-propagation", {}},
        {"grpc-server-headers-propagator", {}},
    };
    return core_pipeline;
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

}  // namespace ugrpc::middlewares::impl

USERVER_NAMESPACE_END
