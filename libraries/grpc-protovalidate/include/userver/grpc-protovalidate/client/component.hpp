#pragma once

/// @file
/// @brief @copybrief grpc_protovalidate::client::ValidatorComponent

#include <userver/ugrpc/client/middlewares/base.hpp>

USERVER_NAMESPACE_BEGIN

/// Client middleware that validates gRPC response constraints specified with the
/// [protovalidate](https://github.com/bufbuild/protovalidate) options in the *proto* files.
namespace grpc_protovalidate::client {

/// @ingroup userver_components
///
/// @brief Component for gRPC client response validation.
///
/// This component checks whether gRPC response satisfies constraints specified in the *proto* file
/// using [protovalidate](https://github.com/bufbuild/protovalidate) syntax. If it does not, middleware
/// throws grpc_protovalidate::client::ResponseError with information about violated constraints.
///
/// ## Static options:
/// Name | Description | Default value
/// ---- | ----------- | -------------
/// fail-fast | do not check remaining constraints after first error is encountered | false
/// methods | array of per-method middleware options overrides | `[]`
/// methods.name | fully-qualified (with package and service) method name | -
/// methods.fail-fast | see `fail-fast` | -

class ValidatorComponent final : public ugrpc::client::MiddlewareFactoryComponentBase {
public:
    /// @ingroup userver_component_names
    /// @brief The default name of grpc_protovalidate::client::ValidatorComponent.
    static constexpr std::string_view kName = "grpc-client-validator";

    ValidatorComponent(const components::ComponentConfig& config, const components::ComponentContext& context);

    static yaml_config::Schema GetStaticConfigSchema();

    yaml_config::Schema GetMiddlewareConfigSchema() const override;

    std::shared_ptr<const ugrpc::client::MiddlewareBase>
    CreateMiddleware(const ugrpc::client::ClientInfo&, const yaml_config::YamlConfig& middleware_config) const override;
};

}  // namespace grpc_protovalidate::client

template <>
inline constexpr bool components::kHasValidate<grpc_protovalidate::client::ValidatorComponent> = true;

template <>
inline constexpr auto components::kConfigFileMode<grpc_protovalidate::client::ValidatorComponent> =
    ConfigFileMode::kNotRequired;

USERVER_NAMESPACE_END
