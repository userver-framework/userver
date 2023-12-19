#pragma once

#include <userver/components/component.hpp>

#include <ugrpc/server/impl/service_defaults.hpp>
#include <userver/ugrpc/server/server.hpp>
#include <userver/ugrpc/server/service_base.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server::impl {

ServiceDefaults ParseServiceDefaults(
    const yaml_config::YamlConfig& value,
    const components::ComponentContext& context);

server::ServiceConfig ParseServiceConfig(
    const yaml_config::YamlConfig& value,
    const components::ComponentContext& context,
    const ServiceDefaults& defaults);

ServerConfig ParseServerConfig(const yaml_config::YamlConfig& value,
                               const components::ComponentContext& context);

}  // namespace ugrpc::server::impl

USERVER_NAMESPACE_END
