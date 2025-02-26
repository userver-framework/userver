#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include <userver/yaml_config/yaml_config.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::middlewares::impl {

struct BaseMiddlewareConfig final {
    bool enabled{true};
};

BaseMiddlewareConfig Parse(const yaml_config::YamlConfig& value, formats::parse::To<BaseMiddlewareConfig>);

struct MiddlewarePipelineConfig final {
    std::unordered_map<std::string, BaseMiddlewareConfig> middlewares{};
};

MiddlewarePipelineConfig Parse(const yaml_config::YamlConfig& value, formats::parse::To<MiddlewarePipelineConfig>);

const std::unordered_map<std::string, BaseMiddlewareConfig>& UserverMiddlewares();

struct MiddlewareRunnerConfig final {
    std::unordered_map<std::string, yaml_config::YamlConfig> middlewares{};
    bool disable_user_group{false};
    bool disable_all{false};
};

MiddlewareRunnerConfig Parse(const yaml_config::YamlConfig& value, formats::parse::To<MiddlewareRunnerConfig>);

struct MiddlewareEnabled final {
    std::string name{};
    bool enabled{true};
};

bool operator==(const MiddlewareEnabled& l, const MiddlewareEnabled& r);

using MiddlewareOrderedList = std::vector<MiddlewareEnabled>;

}  // namespace ugrpc::middlewares::impl

USERVER_NAMESPACE_END
