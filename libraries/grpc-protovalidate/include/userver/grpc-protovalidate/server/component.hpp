#pragma once

/// @file
/// @brief @copybrief grpc_protovalidate::server::ValidatorComponent

#include <userver/ugrpc/server/middlewares/base.hpp>

USERVER_NAMESPACE_BEGIN

/// @brief Top namespace for the server part of the `grpc-protovalidate` library
namespace grpc_protovalidate::server {

/// @ingroup userver_components
///
/// @brief Component for gRPC server request validation.
///
/// This component checks whether gRPC request satisfies constraints specified in the *proto* file
/// using [protovalidate](https://github.com/bufbuild/protovalidate) syntax. If it does not, middleware
/// stops further request handling and returns @c INVALID_ARGUMENT error. Also in that case validator adds
/// @c Violations message filled by protovalidate (see *validate.proto* file) to the @c details field of
/// the @c google.rpc.Status message which is sent sent as @c gprc::Status error details.
///
/// ## Static options:
/// Name | Description | Default value
/// ---- | ----------- | -------------
/// fail-fast | do not check remaining constraints after first error is encountered | true
/// send-violations | send found constraint violations in `grpc::Status` | false
/// methods | array of per-method middleware options overrides | `[]`
/// methods.name | fully-qualified (with package and service) method name | -
/// methods.fail-fast | see `fail-fast` | -
/// methods.send-violations | see `send-violations` | -

class ValidatorComponent final : public ugrpc::server::MiddlewareFactoryComponentBase {
public:
    /// @ingroup userver_component_names
    /// @brief The default name of the grpc_protovalidate::server::ValidatorComponent
    static constexpr std::string_view kName = "grpc-server-validator";

    ValidatorComponent(const components::ComponentConfig& config, const components::ComponentContext& context);

    static yaml_config::Schema GetStaticConfigSchema();

    yaml_config::Schema GetMiddlewareConfigSchema() const override;

    std::shared_ptr<const ugrpc::server::MiddlewareBase> CreateMiddleware(
        const ugrpc::server::ServiceInfo&,
        const yaml_config::YamlConfig& middleware_config
    ) const override;
};

}  // namespace grpc_protovalidate::server

template <>
inline constexpr bool components::kHasValidate<grpc_protovalidate::server::ValidatorComponent> = true;

template <>
inline constexpr auto components::kConfigFileMode<grpc_protovalidate::server::ValidatorComponent> =
    ConfigFileMode::kNotRequired;

USERVER_NAMESPACE_END
