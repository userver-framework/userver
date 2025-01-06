#pragma once

/// @file userver/ugrpc/server/middlewares/log/component.hpp
/// @brief @copybrief ugrpc::server::middlewares::log::Component

#include <userver/ugrpc/server/middlewares/base.hpp>
#include <userver/utils/box.hpp>

USERVER_NAMESPACE_BEGIN

/// Server logging middleware
namespace ugrpc::server::middlewares::log {

struct Settings;

// clang-format off

/// @ingroup userver_components userver_base_classes
///
/// @brief Component for gRPC server logging
///
/// ## Static options:
/// Name | Description | Default value
/// ---- | ----------- | -------------
/// log-level | log level for msg logging | debug
/// msg-log-level | gRPC message body logging level | debug
/// msg-size-log-limit | max message size to log, the rest will be truncated | 512
/// trim-secrets | trim the secrets from logs as marked by the protobuf option | true (*)
///
/// @warning * Trimming secrets causes a segmentation fault for messages that contain
/// optional fields in protobuf versions prior to 3.13. You should set trim-secrets to false
/// if this is the case for you. See https://github.com/protocolbuffers/protobuf/issues/7801
///
/// ## Static configuration example:
///
/// @snippet grpc/functional_tests/basic_chaos/static_config.yaml Sample grpc server logging middleware component config

// clang-format on

class Component final : public MiddlewareComponentBase {
public:
    /// @ingroup userver_component_names
    /// @brief The default name of ugrpc::server::middlewares::log::Component
    static constexpr std::string_view kName = "grpc-server-logging";

    Component(const components::ComponentConfig& config, const components::ComponentContext& context);

    ~Component() override;

    std::shared_ptr<MiddlewareBase> GetMiddleware() override;

    static yaml_config::Schema GetStaticConfigSchema();

private:
    const utils::Box<Settings> settings_;
};

}  // namespace ugrpc::server::middlewares::log

USERVER_NAMESPACE_END
