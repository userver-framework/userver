#pragma once

/// @file userver/ugrpc/client/auth_type.hpp
/// @brief @copybrief ugrpc::client::AuthType

#include <userver/yaml_config/fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client {

/// Authentication method
enum class AuthType {
    kInsecure,
    kSsl,
};

AuthType Parse(const yaml_config::YamlConfig& value, formats::parse::To<AuthType>);

}  // namespace ugrpc::client

USERVER_NAMESPACE_END
