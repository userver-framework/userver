#pragma once

#include <userver/chaotic/openapi/client/command_control.hpp>
#include <userver/chaotic/openapi/client/middleware.hpp>
#include <userver/chaotic/openapi/client/middleware_factory.hpp>
#include <userver/clients/http/request.hpp>
#include <userver/components/component_base.hpp>
#include <userver/components/component_context.hpp>
#include <userver/dynamic_config/snapshot.hpp>
#include <userver/dynamic_config/source.hpp>
#include <userver/dynamic_config/updater/component.hpp>
#include <userver/http/url.hpp>
#include <userver/logging/log.hpp>
#include <userver/yaml_config/schema.hpp>

USERVER_NAMESPACE_BEGIN

namespace chaotic::openapi {

using ConfigKey = const dynamic_config::Key<client::CommandControlDict>;

class QosMiddleware final : public client::Middleware {
public:
    explicit QosMiddleware(dynamic_config::Source source, ConfigKey& key);

    void OnRequest(clients::http::Request& request) override;
    void OnResponse(clients::http::Response&) override;

    static std::string GetStaticConfigSchemaStr();

private:
    dynamic_config::Source source_;
    ConfigKey& key_;
};

template <ConfigKey&>
class QosMiddlewareFactory final : public client::MiddlewareFactory {
public:
    QosMiddlewareFactory(const components::ComponentConfig& config, const components::ComponentContext& context);

    std::shared_ptr<client::Middleware> Create(const yaml_config::YamlConfig& config) override;

    std::string GetStaticConfigSchemaStr() override;

private:
    dynamic_config::Source config_source_;
};

template <ConfigKey& Key>
QosMiddlewareFactory<Key>::QosMiddlewareFactory(
    const components::ComponentConfig& config,
    const components::ComponentContext& context
)
    : client::MiddlewareFactory(config, context),
      config_source_(context.FindComponent<components::DynamicConfig>().GetSource()) {}

template <ConfigKey& Key>
std::shared_ptr<client::Middleware> QosMiddlewareFactory<Key>::Create(const yaml_config::YamlConfig&) {
    return std::make_shared<QosMiddleware>(config_source_, Key);
}

template <ConfigKey& Key>
std::string QosMiddlewareFactory<Key>::GetStaticConfigSchemaStr() {
    return QosMiddleware::GetStaticConfigSchemaStr();
}

}  // namespace chaotic::openapi

USERVER_NAMESPACE_END

template <USERVER_NAMESPACE::chaotic::openapi::ConfigKey& Key>
inline constexpr auto
    USERVER_NAMESPACE::components::kConfigFileMode<USERVER_NAMESPACE::chaotic::openapi::QosMiddlewareFactory<Key>> =
        USERVER_NAMESPACE::components::ConfigFileMode::kNotRequired;
