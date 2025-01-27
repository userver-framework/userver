#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include <userver/yaml_config/fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server::impl {

struct MiddlewareConfig final {
    bool enabled{true};
};

MiddlewareConfig Parse(const yaml_config::YamlConfig& value, formats::parse::To<MiddlewareConfig>);

struct MiddlewarePipelineConfig final {
    std::unordered_map<std::string, MiddlewareConfig> middlewares{};
};

MiddlewarePipelineConfig Parse(const yaml_config::YamlConfig& value, formats::parse::To<MiddlewarePipelineConfig>);

const std::unordered_map<std::string, MiddlewareConfig>& UserverMiddlewares();

struct MiddlewareServiceConfig final {
    std::unordered_map<std::string, MiddlewareConfig> service_middlewares{};
    bool disable_user_pipeline_middlewares{false};
    bool disable_all_pipeline_middlewares{false};
};

MiddlewareServiceConfig Parse(const yaml_config::YamlConfig& value, formats::parse::To<MiddlewareServiceConfig>);

struct MiddlewareEnabled final {
    std::string name{};
    bool enabled{true};
};

bool operator==(const MiddlewareEnabled& l, const MiddlewareEnabled& r);

using MiddlewareOrderedList = std::vector<MiddlewareEnabled>;

}  // namespace ugrpc::server::impl

USERVER_NAMESPACE_END
