#include <userver/ugrpc/server/impl/middleware_pipeline_config.hpp>

#include <userver/yaml_config/yaml_config.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server::impl {

MiddlewarePipelineConfig Parse(const yaml_config::YamlConfig& value, formats::parse::To<MiddlewarePipelineConfig>) {
    MiddlewarePipelineConfig config;
    config.middlewares = value["middlewares"].As<std::unordered_map<std::string, MiddlewareConfig>>({});
    return config;
}

const std::unordered_map<std::string, MiddlewareConfig>& UserverMiddlewares() {
    static std::unordered_map<std::string, MiddlewareConfig> core_pipeline{
        {"grpc-server-logging", {}},
        {"grpc-server-baggage", {}},
        {"grpc-server-congestion-control", {}},
        {"grpc-server-deadline-propagation", {}},
        {"grpc-server-field-mask", {}},
        {"grpc-server-headers-propagator", {}},
    };
    return core_pipeline;
}

MiddlewareConfig Parse(const yaml_config::YamlConfig& value, formats::parse::To<MiddlewareConfig>) {
    MiddlewareConfig config{};
    config.enabled = value["enabled"].As<bool>(config.enabled);
    return config;
}

MiddlewareServiceConfig Parse(const yaml_config::YamlConfig& value, formats::parse::To<MiddlewareServiceConfig>) {
    MiddlewareServiceConfig conf{};
    conf.disable_user_pipeline_middlewares =
        value["disable-user-pipeline-middlewares"].As<bool>(conf.disable_user_pipeline_middlewares);
    conf.disable_all_pipeline_middlewares =
        value["disable-all-pipeline-middlewares"].As<bool>(conf.disable_all_pipeline_middlewares);
    conf.service_middlewares = value["middlewares"].As<std::unordered_map<std::string, MiddlewareConfig>>({});
    return conf;
}

bool operator==(const MiddlewareEnabled& l, const MiddlewareEnabled& r) {
    return l.name == r.name && l.enabled == r.enabled;
}

}  // namespace ugrpc::server::impl

USERVER_NAMESPACE_END
