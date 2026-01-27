#include <userver/ugrpc/server/middlewares/log/component.hpp>

#include <userver/components/component_config.hpp>
#include <userver/formats/parse/common_containers.hpp>
#include <userver/logging/level_serialization.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

#include <ugrpc/server/middlewares/log/middleware.hpp>
#include <userver/middlewares/groups.hpp>
#include <userver/ugrpc/server/middlewares/access_log/component.hpp>
#include <userver/ugrpc/status_codes.hpp>

#ifndef ARCADIA_ROOT
#include "generated/src/ugrpc/server/middlewares/log/component.yaml.hpp"  // Y_IGNORE
#endif

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server::middlewares::log {

Settings Parse(const yaml_config::YamlConfig& config, formats::parse::To<Settings>) {
    Settings settings;
    settings.log_level = config["log-level"].As<logging::Level>(settings.log_level);
    settings.msg_log_level = config["msg-log-level"].As<logging::Level>(settings.msg_log_level);
    settings.max_msg_size = config["msg-size-log-limit"].As<std::size_t>(settings.max_msg_size);
    settings.local_log_level = config["local-log-level"].As<logging::Level>(settings.local_log_level);
    settings.status_codes_log_level =
        config["status-codes-log-level"].As<boost::container::flat_map<grpc::StatusCode, logging::Level>>({});
    return settings;
}

/// [middleware InGroup]
Component::Component(const components::ComponentConfig& config, const components::ComponentContext& context)
    : MiddlewareFactoryComponentBase(
          config,
          context,
          USERVER_NAMESPACE::middlewares::MiddlewareDependencyBuilder()
              .InGroup<USERVER_NAMESPACE::middlewares::groups::Logging>()
              .After<access_log::Component>(USERVER_NAMESPACE::middlewares::DependencyType::kWeak)
      )
{}
/// [middleware InGroup]

std::shared_ptr<const MiddlewareBase> Component::CreateMiddleware(
    const ugrpc::server::ServiceInfo&,
    const yaml_config::YamlConfig& middleware_config
) const {
    return std::make_shared<Middleware>(middleware_config.As<Settings>());
}

yaml_config::Schema Component::GetMiddlewareConfigSchema() const { return GetStaticConfigSchema(); }

yaml_config::Schema Component::GetStaticConfigSchema() {
    return yaml_config::MergeSchemasFromResource<
        MiddlewareFactoryComponentBase>("src/ugrpc/server/middlewares/log/component.yaml");
}

}  // namespace ugrpc::server::middlewares::log

USERVER_NAMESPACE_END
