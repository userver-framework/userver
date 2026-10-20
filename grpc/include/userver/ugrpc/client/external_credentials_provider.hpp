#pragma once

/// @file userver/ugrpc/client/external_credentials_provider.hpp
/// @brief @copybrief ugrpc::client::ExternalCredentialsProvider

#include <grpcpp/security/credentials.h>
#include <optional>
#include <string_view>
#include <userver/components/component_base.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client {

// clang-format off

/// @ingroup userver_components
///
/// @brief Provides GRPC SSL credentials options to @ref ugrpc::client::ClientFactoryComponent
/// Should be implemented by userver framework client as a component.

// clang-format on

class ExternalCredentialsProvider : public components::ComponentBase {
public:
    static constexpr std::string_view kName = "external-grpc-client-credentials-provider";

    using components::ComponentBase::ComponentBase;

    /// The method is called by @ref ugrpc::client::ClientFactoryComponent.
    /// Implement the method, in order to override SSL credentials specified in GRPC client factory config.
    /// Returned credentials have a precedence over the credentials from GPRC client factory config.
    /// If returned value is not `std::nullopt`, SSL is turned on with the provided credentials in GRPC client factory.
    /// Otherwise, SSL credentials from GRPC client factory config is used.
    virtual std::optional<grpc::SslCredentialsOptions> GetSslCredentialsOptions() = 0;
};

}  // namespace ugrpc::client

USERVER_NAMESPACE_END
