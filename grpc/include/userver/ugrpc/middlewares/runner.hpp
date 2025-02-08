#pragma once

/// @file userver/ugrpc/middlewares/runner.hpp
/// @brief @copybrief ugrpc::middlewares::RunnerComponentBase

#include <vector>

#include <userver/components/component_base.hpp>
#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

#include <userver/ugrpc/impl/middleware_pipeline_config.hpp>
#include <userver/ugrpc/middlewares/pipeline.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::middlewares {

namespace impl {

yaml_config::YamlConfig MakeMiddlewareConfig(
    const formats::yaml::Value& global,
    const yaml_config::YamlConfig& local,
    yaml_config::Schema schema
);

/// Make the default builder for `group::User` group.
MiddlewareDependencyBuilder MakeDefaultUserDependency();

}  // namespace impl

/// @ingroup userver_base_classes
///
/// @brief Base class for middleware factory component.
template <typename MiddlewareBaseType, typename HandlerInfo>
class MiddlewareFactoryComponentBase : public components::ComponentBase {
public:
    using MiddlewareBase = MiddlewareBaseType;

    MiddlewareFactoryComponentBase(
        const components::ComponentConfig& config,
        const components::ComponentContext& context,
        MiddlewareDependencyBuilder&& builder = impl::MakeDefaultUserDependency()
    )
        : components::ComponentBase(config, context),
          dependency_(std::move(builder).Extract(config.Name())),
          global_config_(config.As<formats::yaml::Value>()) {}

    /// @brief Returns a middleware according to the component's settings.
    ///
    /// @param info is a handler info for the middleware.
    /// @param middleware_config config for the middleware.
    virtual std::shared_ptr<MiddlewareBase>
    CreateMiddleware(const HandlerInfo& info, const yaml_config::YamlConfig& middleware_config) const = 0;

    /// @brief This method should return the schema of a middleware configuration.
    /// Always write `return GetStaticConfigSchema();` in this method.
    virtual yaml_config::Schema GetMiddlewareConfigSchema() const { return GetStaticConfigSchema(); }

    static yaml_config::Schema GetStaticConfigSchema() {
        return yaml_config::MergeSchemas<components::ComponentBase>(R"(
type: object
description: base class for grpc-server middleware
additionalProperties: false
properties:
    enabled:
        type: string
        description: the flag to enable/disable middleware in the pipeline
        defaultDescription: true
)");
    }

    /// @cond
    /// Only for internal use.
    const ugrpc::middlewares::impl::MiddlewareDependency& GetMiddlewareDependency(utils::impl::InternalTag) const {
        return dependency_;
    }

    const formats::yaml::Value& GetGlobalConfig(utils::impl::InternalTag) const { return global_config_; }
    /// @endcond

private:
    const ugrpc::middlewares::impl::MiddlewareDependency dependency_;
    const formats::yaml::Value global_config_;
};

/// @brief Base class for a component that runs middlewares.
///
/// There are a local and global configs of middlewares.
/// Global config of middleware is a classic config in `components_manager.components`.
/// You can override the global by local config in the config of this component. See the middlewares option.
///
/// RunnerComponentBase creates middleware instances using `MiddlewareFactoryComponentBase`.
template <typename MiddlewareBase, typename HandlerInfo>
class RunnerComponentBase : public components::ComponentBase {
public:
    static yaml_config::Schema GetStaticConfigSchema() {
        return yaml_config::MergeSchemas<components::ComponentBase>(R"(
type: object
description: base class for all the gRPC service components
additionalProperties: false
properties:
    disable-user-pipeline-middlewares:
        type: boolean
        description: flag to disable groups::User middlewares from pipeline
        defaultDescription: false
    disable-all-pipeline-middlewares:
        type: boolean
        description: flag to disable all middlewares from pipline
        defaultDescription: false
    middlewares:
        type: object
        description: overloads of configs of middlewares per service
        additionalProperties:
            type: object
            description: a middleware config
            additionalProperties: true
            properties:
                enabled:
                    type: boolean
                    description: enable middleware in the list
        properties: {}
)");
    }

protected:
    using MiddlewareFactory = MiddlewareFactoryComponentBase<MiddlewareBase, HandlerInfo>;

    RunnerComponentBase(
        const components::ComponentConfig& config,
        const components::ComponentContext& context,
        std::string_view pipeline_component_name
    )
        : components::ComponentBase(config, context) {
        const auto& middlewares = config["middlewares"];
        const auto& pipeline =
            context.FindComponent<middlewares::MiddlewarePipelineComponent>(pipeline_component_name).GetPipeline();
        for (const auto& mid : pipeline.GetPerServiceMiddlewares(config.As<impl::MiddlewareRunnerConfig>())) {
            const auto* factory = context.FindComponentOptional<MiddlewareFactory>(mid);
            UINVARIANT(factory != nullptr, "Middleware must be exists");
            middleware_infos_.push_back(MiddlewareInfo{factory, middlewares[mid]});
        }
    }

    /// @cond
    /// Only for internal use.
    std::vector<std::shared_ptr<MiddlewareBase>> CreateMiddlewares(const HandlerInfo& info) {
        std::vector<std::shared_ptr<MiddlewareBase>> middlewares;
        middlewares.reserve(middleware_infos_.size());
        for (const auto& [factory, local_config] : middleware_infos_) {
            auto config = impl::MakeMiddlewareConfig(
                factory->GetGlobalConfig(utils::impl::InternalTag{}), local_config, factory->GetMiddlewareConfigSchema()
            );
            middlewares.push_back(factory->CreateMiddleware(info, config));
        }
        middleware_infos_.clear();
        return middlewares;
    }
    /// @endcond

private:
    struct MiddlewareInfo final {
        const MiddlewareFactory* const factory{nullptr};
        yaml_config::YamlConfig local_config{};
    };

    std::vector<MiddlewareInfo> middleware_infos_{};
};

}  // namespace ugrpc::middlewares

USERVER_NAMESPACE_END
