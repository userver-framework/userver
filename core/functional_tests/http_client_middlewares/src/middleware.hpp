#pragma once

#include <userver/clients/http/middlewares/base.hpp>
#include <userver/clients/http/middlewares/component.hpp>
#include <userver/clients/http/response.hpp>
#include <userver/components/component.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

class Middleware final : public clients::http::MiddlewareBase {
public:
    Middleware(std::string key, std::string value)
        : clients::http::MiddlewareBase(),
          key_(std::make_unique<std::string>(std::move(key))),
          value_(std::make_unique<std::string>(std::move(value)))
    {}

    void HookPerformRequest(clients::http::MiddlewareRequest& request) override { request.SetHeader(*key_, *value_); }

    void HookOnCompleted(clients::http::MiddlewareRequest&, clients::http::Response& response) override {
        response.headers().emplace(*key_, *value_);
    }

private:
    std::unique_ptr<std::string> key_;
    std::unique_ptr<std::string> value_;
};

class MiddlewareComponent final : public clients::http::middlewares::ComponentBase {
public:
    static constexpr std::string_view kName = "http-client-add-header";

    MiddlewareComponent(const components::ComponentConfig& config, const components::ComponentContext& context)
        : ComponentBase(config, context, clients::http::middlewares::MiddlewareIndex{0}),
          middleware_(std::make_unique<Middleware>(config["key"].As<std::string>(), config["value"].As<std::string>()))
    {}

    ~MiddlewareComponent() override = default;

    clients::http::MiddlewareBase& GetMiddleware() override { return *middleware_; }
    static yaml_config::Schema GetStaticConfigSchema() {
        return yaml_config::MergeSchemas<clients::http::middlewares::ComponentBase>(R"(
    type: object
    description: Test middleware adding header to request and response
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
    std::unique_ptr<Middleware> middleware_;
};
