#pragma once

/// @file userver/middlewares/runner.hpp
/// @brief @copybrief middlewares::RunnerComponentBase

#include <vector>

#include <userver/components/component_base.hpp>
#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/impl/internal_tag.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

#include <userver/middlewares/impl/middleware_pipeline_config.hpp>
#include <userver/middlewares/impl/pipeline_creator_interface.hpp>
#include <userver/middlewares/pipeline.hpp>

USERVER_NAMESPACE_BEGIN

namespace middlewares {

namespace impl {

// Options for a middleware can be from two places:
// 1. The global config of a middleware.
// 2. Overriding in RunnerComponentBase component.
// We must validate (2) and merge values from (1) and (2).
yaml_config::YamlConfig ValidateAndMergeMiddlewareConfigs(
    const formats::yaml::Value& global,
    const yaml_config::YamlConfig& local,
    yaml_config::Schema schema
);

/// Make the default builder for `middlewares::groups::User` group.
MiddlewareDependencyBuilder MakeDefaultUserDependency();

class WithMiddlewareDependencyComponentBase : public components::ComponentBase {
public:
    WithMiddlewareDependencyComponentBase(
        const components::ComponentConfig& config,
        const components::ComponentContext& context
    )
        : components::ComponentBase(config, context) {}

    virtual const middlewares::impl::MiddlewareDependency& GetMiddlewareDependency(utils::impl::InternalTag) const = 0;
};

void LogConfiguration(std::string_view component_name, const std::vector<std::string>& names);

void LogValidateError(std::string_view middleware_name, const std::exception& e);

}  // namespace impl

/// @ingroup userver_base_classes
///
/// @brief Base class for middleware factory component.
template <typename MiddlewareBaseType, typename HandlerInfo>
class MiddlewareFactoryComponentBase : public impl::WithMiddlewareDependencyComponentBase {
public:
    using MiddlewareBase = MiddlewareBaseType;

    MiddlewareFactoryComponentBase(
        const components::ComponentConfig& config,
        const components::ComponentContext& context,
        MiddlewareDependencyBuilder&& builder = impl::MakeDefaultUserDependency()
    )
        : impl::WithMiddlewareDependencyComponentBase(config, context),
          global_config_(config.As<formats::yaml::Value>()),
          dependency_(std::move(builder).ExtractDependency(/*middleware_name=*/config.Name())) {}

    /// @brief Returns a middleware according to the component's settings.
    ///
    /// @param info is a handler info for the middleware.
    /// @param middleware_config config for the middleware.
    ///
    /// @warning Don't store `info` by reference. `info` object will be dropped after the `CreateMiddleware` call.
    virtual std::shared_ptr<const MiddlewareBase>
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
    const middlewares::impl::MiddlewareDependency& GetMiddlewareDependency(utils::impl::InternalTag) const override {
        return dependency_;
    }

    const formats::yaml::Value& GetGlobalConfig(utils::impl::InternalTag) const { return global_config_; }
    /// @endcond

private:
    const formats::yaml::Value global_config_;
    middlewares::impl::MiddlewareDependency dependency_;
};

/// @brief Base class for a component that runs middlewares.
///
/// There are a local and global configs of middlewares.
/// Global config of middleware is a classic config in `components_manager.components`.
/// You can override the global config for the specific service/client by the local config in the config of this
/// component: see the 'middlewares' option.
///
/// `RunnerComponentBase` creates middleware instances using `MiddlewareFactoryComponentBase`.
/// The Ordered list of middlewares `RunnerComponentBase` takes from Pipeline component.
/// So, 'Pipeline' is responsible for the order of middlewares. `RunnerComponentBase` is responsible for creating
/// middlewares and overriding configs.
template <typename MiddlewareBase, typename HandlerInfo>
class RunnerComponentBase : public components::ComponentBase,
                            public impl::PipelineCreatorInterface<MiddlewareBase, HandlerInfo> {
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
        description: flag to disable all middlewares from pipeline
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
        std::string_view pipeline_name
    );

    /// @cond
    // Only for internal use.
    std::vector<std::shared_ptr<const MiddlewareBase>> CreateMiddlewares(const HandlerInfo& info) const override;
    /// @endcond

private:
    struct MiddlewareInfo final {
        const MiddlewareFactory* factory{nullptr};
        yaml_config::YamlConfig local_config{};
    };

    std::vector<MiddlewareInfo> middleware_infos_{};
};

template <typename MiddlewareBase, typename HandlerInfo>
RunnerComponentBase<MiddlewareBase, HandlerInfo>::RunnerComponentBase(
    const components::ComponentConfig& config,
    const components::ComponentContext& context,
    std::string_view pipeline_name
)
    : components::ComponentBase(config, context) {
    const auto& middlewares = config["middlewares"];
    const auto& pipeline = context.FindComponent<impl::AnyMiddlewarePipelineComponent>(pipeline_name).GetPipeline();
    const auto names = pipeline.GetPerServiceMiddlewares(config.As<impl::MiddlewareRunnerConfig>());
    impl::LogConfiguration(config.Name(), names);
    for (const auto& mid : names) {
        const auto* factory = context.FindComponentOptional<MiddlewareFactory>(mid);
        UINVARIANT(factory != nullptr, fmt::format("The middleware '{}' must exist", mid));
        middleware_infos_.push_back(MiddlewareInfo{factory, middlewares[mid]});
    }
}

/// @cond
template <typename MiddlewareBase, typename HandlerInfo>
std::vector<std::shared_ptr<const MiddlewareBase>> RunnerComponentBase<MiddlewareBase, HandlerInfo>::CreateMiddlewares(
    const HandlerInfo& info
) const {
    std::vector<std::shared_ptr<const MiddlewareBase>> middlewares{};
    middlewares.reserve(middleware_infos_.size());
    for (const auto& [factory, local_config] : middleware_infos_) {
        try {
            auto config = impl::ValidateAndMergeMiddlewareConfigs(
                factory->GetGlobalConfig(utils::impl::InternalTag{}), local_config, factory->GetMiddlewareConfigSchema()
            );
            middlewares.push_back(factory->CreateMiddleware(info, config));
        } catch (const std::exception& e) {
            impl::LogValidateError(factory->GetMiddlewareDependency(utils::impl::InternalTag{}).middleware_name, e);
            throw;
        }
    }
    return middlewares;
}
/// @endcond

namespace impl {

/// @brief A short-cut for defining a middleware-factory.
///
/// `MiddlewareBase` type is a interface of middleware, so `Middleware` type is a implementation of this interface.
/// Your middleware will be created with a default constructor and will be in `middlewares::groups::User` group.
template <typename MiddlewareBase, typename Middleware, typename HandlerInfo>
class SimpleMiddlewareFactoryComponent final : public MiddlewareFactoryComponentBase<MiddlewareBase, HandlerInfo> {
public:
    static constexpr std::string_view kName = Middleware::kName;

    SimpleMiddlewareFactoryComponent(
        const components::ComponentConfig& config,
        const components::ComponentContext& context
    )
        : MiddlewareFactoryComponentBase<MiddlewareBase, HandlerInfo>(
              config,
              context,
              middlewares::MiddlewareDependencyBuilder{Middleware::kDependency}
          ) {}

private:
    std::shared_ptr<const MiddlewareBase> CreateMiddleware(const HandlerInfo&, const yaml_config::YamlConfig&)
        const override {
        return std::make_shared<Middleware>();
    }
};

}  // namespace impl

}  // namespace middlewares

template <typename MiddlewareBase, typename Middleware, typename HandlerInfo>
inline constexpr bool components::kHasValidate<
    middlewares::impl::SimpleMiddlewareFactoryComponent<MiddlewareBase, Middleware, HandlerInfo>> = true;

USERVER_NAMESPACE_END
