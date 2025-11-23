#pragma once

#include <userver/clients/http/plugin.hpp>
#include <userver/clients/http/plugin_component.hpp>
#include <userver/clients/http/response.hpp>
#include <userver/components/component.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

class Plugin final : public clients::http::Plugin {
public:
    Plugin(std::string key, std::string value)
        : clients::http::Plugin("add-header"),
          key_(std::make_unique<std::string>(std::move(key))),
          value_(std::make_unique<std::string>(std::move(value)))
    {}

    void HookPerformRequest(clients::http::PluginRequest& request) override { request.SetHeader(*key_, *value_); }

    void HookCreateSpan(clients::http::PluginRequest&, tracing::Span&) override {}

    void HookOnCompleted(clients::http::PluginRequest&, clients::http::Response& response) override {
        response.headers().emplace(*key_, *value_);
    }

    void HookOnError(clients::http::PluginRequest&, std::error_code) override {}

    bool HookOnRetry(clients::http::PluginRequest&) override { return true; }

private:
    std::unique_ptr<std::string> key_;
    std::unique_ptr<std::string> value_;
};

class PluginComponent final : public clients::http::plugin::ComponentBase {
public:
    static constexpr std::string_view kName = "http-client-plugin-add-header";

    PluginComponent(const components::ComponentConfig& config, const components::ComponentContext& context)
        : ComponentBase(config, context),
          plugin_(std::make_unique<Plugin>(config["key"].As<std::string>(), config["value"].As<std::string>()))
    {}

    ~PluginComponent() override = default;

    clients::http::Plugin& GetPlugin() override { return *plugin_; }
    static yaml_config::Schema GetStaticConfigSchema() {
        return yaml_config::MergeSchemas<clients::http::plugin::ComponentBase>(R"(
    type: object
    description: Test plugin adding header to request and response
    additionalProperties: false
    properties:
        key:
            type: string
            description: header name
        value:
            type: string
            description: header value
    )");
    }

private:
    std::unique_ptr<Plugin> plugin_;
};
